[TOC]



## src

### config

```c++
bool parse_unsigned(const std::string& text, unsigned int& value) {
    unsigned int parsed = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc() || result.ptr != end) {
        return false;
    }
    value = parsed;
    return true;
}
```

字符串转无符号整数

**传统 `std::stoi` 的痛点**：如果客户端发来一个空串 `""` 或超长数字，`std::stoi` 会直接**抛出异常**

**你的函数优势**：返回 `bool`，不抛异常。上层代码可以稳稳地捕获失败情况，回复客户端“格式错误”而不是挂掉

-   **传统 `std::atoi` 或 `stoi` 的漏洞**：它们遇到 `"123abc"` 会静默返回 `123`，并忽略 `abc`。

-   **为什么这很危险**：假设你的聊天协议里有一个字段是“房间号”，客户端恶意发送 `"1DROP TABLE users"`（虽然这里是数字字段，但作为字符串解析的例子）。如果解析器允许尾随字符，你的业务层可能只读到 `1`，但底层 Buffer 处理残留数据时，可能会把 `DROP` 当作下一条命令解析，引发**协议错位**或逻辑漏洞。

-   **你的函数严格性**：`result.ptr != end` 这条校验，强制要求**字符串必须全是数字，不能有多余的字母、空格或控制字符**，彻底杜绝了这类脏数据混入。

-   **性能对比**：

    -   `std::stringstream`：涉及内存动态分配和本地化（locale）处理，极慢。
    -   `std::stoi`：内部依赖 `strtol`，需要处理 C 语言环境，且包含异常处理机制（即使没抛出，编译器插入了大量栈展开代码）。
    -   **`std::from_chars`**：是 C++17 引入的**纯头文件+编译器内置**函数，**不分配内存、不抛异常、不依赖本地化**，是目前 C++ 中最快的整数转换方案。对于高吞吐量的 `TcpServer`，这种微小的延迟节省累积起来效果显著。

    -   使用传统 `atoi`，如果客户端传空串，返回 `0`，上层逻辑会误认为是在查询 `ID=0` 的用户，导致不可预知的错误（比如查错数据）。
    -   你的函数通过 `bool` 返回值 **将“解析成功/失败”与“数值本身”解耦**，业务层可以精准区分：

    使用：

    -   **`src/protocol.cpp`**（解析网络包中的长度字段、命令序号）。
    -   **`src/config.cpp`**（解析配置文件中的端口号、线程数）。
    -   **`src/mysql_database.cpp`**（解析查询结果集中的整型字段）。

    它就像是**网络数据进入核心业务逻辑前的一道“安检门”**，保证了进到内存中的数字一定是合法、纯净、安全的，支撑了整个 `minimuduo` 高并发模型的稳定运行。

    

    

### mysql_database

```c
using ResultPtr =
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
```

**一个使用 `mysql_free_result` 作为自定义删除器的独占智能指针，用于自动管理 `MYSQL_RES\*`（MySQL 查询结果集）的内存释放**

-   **`MYSQL_RES`**：MySQL C API 中定义的结果集结构体类型。当你执行 `SELECT` 后，`mysql_store_result()` 返回的就是 `MYSQL_RES*`。
-   **`decltype(&mysql_free_result)`**：获取 `mysql_free_result` 这个函数的**类型**（函数指针类型）。在 C 语言中，`mysql_free_result` 的签名是 `void mysql_free_result(MYSQL_RES *result)`，其函数指针类型就是 `void (*)(MYSQL_RES*)`。
-   **`std::unique_ptr<T, Deleter>`**：C++11 提供的独占智能指针。当智能指针生命周期结束（离开作用域或被重置）时，会自动调用指定的 `Deleter` 来销毁资源。



```c++
std::string mysql_error_text(MYSQL* connection) {
    return connection != nullptr
        ? mysql_error(connection)
        : "MySQL connection is null";
}
```

-   **原生风险**：`mysql_error(connection)` 要求传入的 `connection` 必须是一个有效的 `MYSQL*` 指针。如果不小心传入了 `nullptr`，程序会立即**段错误（Segmentation Fault）** 崩溃。
-   **你的函数处理**：通过 `connection != nullptr` 判断，如果连接为空，不调用底层 API，而是直接返回清晰的字符串 `"MySQL connection is null"`。这在调试阶段非常友好，避免了服务器意外宕机
-   将 C 字符串转为安全的 C++ `std::string`，方便上层日志记录。

```c++
class MySqlThreadGuard {
public:
    MySqlThreadGuard() {
        (void)mysql_thread_init();
    }

    ~MySqlThreadGuard() {
        mysql_thread_end();
    }
};
```

在你调用 `mysql_query()`、`mysql_store_result()` 之前，MySQL C 库（`libmysqlclient`）需要在当前线程中分配一些**线程局部存储（TLS, Thread Local Storage）**，比如：

-   **错误缓冲区**：每个线程必须有自己的 `mysql_error()` 存储空间，否则多线程同时报错会互相覆盖。

**如果不调用它**：在多线程环境下，第一个 MySQL 操作（如 `mysql_real_connect`）可能会**隐式地自动调用**它。但依赖这种隐式行为是不安全的，尤其是在高并发创建/销毁线程的场景下，容易导致内存泄漏或竞态条件。因此，**显式调用**是防御性编程的规范做法。

**2. 为什么把它放在构造函数里？（RAII 设计思想）**

这通常是一个**守卫类（Guard Class）**，配合析构函数使用

在你的聊天服务器中，每个 Reactor 线程（或工作线程）在启动时，只需要在栈上创建这个对象：

```cpp
void worker_thread_routine() {
    MySqlThreadGuard mysql_guard;  // 一进入线程，自动调用 mysql_thread_init()
    // ... 执行各种数据库查询 ...
    // 线程结束时，mysql_guard 析构，自动调用 mysql_thread_end()
}
```

**好处**：利用 C++ 的 RAII（资源获取即初始化），保证无论线程是正常结束还是抛出异常，MySQL 的线程清理函数都会被调用，彻底杜绝线程局部资源泄漏。

```c++
void ensure_mysql_thread() {
    thread_local MySqlThreadGuard guard;
    (void)guard;
}
```

-   **`thread_local`**：这是一个 C++11 引入的关键字。它告诉编译器，`guard` 这个变量**不是普通的栈变量，而是“每个线程独一份”**。
-   **延迟初始化（Lazy Initialization）**：`thread_local` 变量只会在**当前线程第一次执行到这行代码时**才会被构造（调用 `MySqlThreadGuard` 的构造函数）。
-   **构造函数的“副作用”**：正如你上一个问题中看到的，`MySqlThreadGuard` 的构造函数会调用 `mysql_thread_init()`。所以，调用 `ensure_mysql_thread()` 的目的**不是为了使用 `guard` 变量本身，而是为了触发它的构造过程**。

-   **第一次调用（当前线程）**：
    1.  程序执行 `thread_local MySqlThreadGuard guard;`。
    2.  编译器发现这是该线程第一次遇到此定义，于是立即构造 `guard`。
    3.  调用 `MySqlThreadGuard()` 构造函数 → 调用 `mysql_thread_init()` → 当前线程注册 MySQL TLS 资源。
    4.  继续执行 `(void)guard;`（什么也不做）。
    5.  函数返回，但 `guard` **并没有被销毁**！因为它是 `thread_local`，它的生命周期会持续到**整个线程结束**。
-   **后续再次调用（同一线程）**：
    1.  再次执行到 `thread_local MySqlThreadGuard guard;`。
    2.  编译器检查到该线程已经构造过 `guard`，因此**直接跳过构造**，什么都不做。
    3.  函数立即返回。**开销极小**（相当于一次 TLS 指针查询，比互斥锁快得多）。

```c++
connection_ = mysql_init(nullptr);
```

| 步骤                    | 代码体现                                             | 作用说明                                                     |
| :---------------------- | :--------------------------------------------------- | :----------------------------------------------------------- |
| **0. 环境预备**         | `ensure_mysql_thread();`                             | 确保当前工作线程已初始化 MySQL 的线程局部存储（TLS），防止多线程崩溃。 |
| **1. 线程安全锁**       | `std::lock_guard<std::mutex> lock(mutex_);`          | 由于 `minimuduo` 是多 Reactor 线程，加锁防止多个线程同时操作同一个 `connection_` 句柄导致协议错乱。 |
| **2. 清理旧连接**       | `mysql_close(connection_);`                          | 如果之前已有连接（比如断线重连），先彻底关闭并释放资源，避免内存泄漏和句柄冲突。 |
| **3. 初始化句柄**       | `connection_ = mysql_init(nullptr);`                 | 向 MySQL 客户端库申请一个 `MYSQL` 结构体内存。失败通常意味着内存耗尽。 |
| **4. 设置超时选项**     | `mysql_options(..., MYSQL_OPT_CONNECT_TIMEOUT, ...)` | **非常重要**！设置 TCP 连接超时（你配置的 `connect_timeout_seconds`）。如果不设，默认可能阻塞几十秒，在高并发下会拖垮整个 Reactor 线程。 |
| **5. 发起握手（核心）** | `mysql_real_connect(...)`                            | 这才是真正的**网络三次握手 + MySQL 认证握手**。传入主机、端口、账号、密码、库名。`CLIENT_MULTI_STATEMENTS` 标志允许执行多条 SQL（以分号分隔），增强了协议灵活性。 |
| **6. 设置字符集**       | `mysql_set_character_set(..., "utf8mb4")`            | **极具远见的步骤**！`utf8mb4` 才是真正的 4 字节 UTF-8（支持 Emoji 表情和生僻汉字）。如果不设，默认 `latin1`，存储中文会变乱码 `???`。 |

```c++
bool MySqlDatabase::ping(std::string& error) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    if (connection_ == nullptr || mysql_ping(connection_) != 0) {
        error = mysql_error_text(connection_);
        return false;
    }

    return true;
}
```

你的 `minimuduo` 服务器与 MySQL 建立的是**长连接**（TCP 持久连接）。但存在两个天然隐患：

1.  **MySQL 服务端超时断开**：MySQL 默认 `wait_timeout` 是 8 小时。如果聊天室凌晨无人访问，8 小时后连接会被 MySQL 服务端主动 Kill。当早上第一个用户登录时，如果不做检测，直接执行 `mysql_query` 就会收到 `"MySQL server has gone away"` 并崩溃。
2.  **网络中间设备（防火墙/NAT）超时**：云服务商的 NAT 网关或硬件防火墙会回收长时间无流量的 TCP 连接。

**`ping` 的解决方案**：在每次真正的业务查询（如 `queryUserByName`）之前，或每隔几分钟的定时任务中，先调用 `ping`。如果返回 `false`，上层代码可以调用 `connect` 函数重新建立连接，确保后续 SQL 一定成功。

核心焦点：`mysql_ping(connection_)`

-   **做了什么**：向 MySQL 服务器发送一个 **`COM_PING`** 命令（相当于网络上的 ICMP Ping），服务器收到后会立即回复 `PONG`。
-   **自动重连机制（非常重要！）**：如果 `mysql_ping` 发现 TCP 连接已经断开（或服务器无响应），**MySQL C API 默认会自动调用 `mysql_real_connect` 尝试重新连接**。
-   **返回值**：连接有效返回 `0`，连接失效或重连失败返回非零值。

```c++
std::string MySqlDatabase::escape(const std::string& value) {
    if (connection_ == nullptr) {
        return {};
    }

    std::string escaped(value.size() * 2U + 1U, '\0');//最大长度估算：根据 MySQL 官方文档，最坏情况下（输入全是需要转义的字符，如 '\'），转义后长度最多是原长的 2 倍。+1 是为了预留 NULL 终止符（虽然 std::string 不用 C 字符串结尾，但底层 C API 需要）。

    const unsigned long size = mysql_real_escape_string(
        connection_,
        escaped.data(),
        value.data(),
        static_cast<unsigned long>(value.size())
    );//遍历 value 中的每一个字节，若发现 '、"、\、\n、\r、\0、\x1a（Ctrl-Z）等危险字符，会在前面加一个反斜杠 \。

    escaped.resize(size);//把 std::string 的长度截断到实际转义长度，丢弃之前填充的冗余 \0 字符，保证返回的字符串不包含额外的空字节。
    return escaped;
}
```

**把你业务层传来的原始字符串（如用户名 `O'Reilly`）中的特殊字符（如单引号 `'`、反斜杠 `\`、双引号等）进行转义，使其能安全地拼接到 SQL 语句中，从而避免语法错误和 SQL 注入攻击。**

| 输入 (`value`) | 输出 (`escaped`) | 作用                                                         |
| :------------- | :--------------- | :----------------------------------------------------------- |
| `O'Reilly`     | `O\'Reilly`      | 防止 SQL 中的单引号提前闭合。                                |
| `C:\Users`     | `C:\\Users`      | 防止 `\U` 被 MySQL 误解为 Unicode 转义。                     |
| `100%`         | `100\%`          | （如开启 `NO_BACKSLASH_ESCAPES` 模式时）防止 `%` 在 LIKE 查询中被当作通配符。 |

```c#
exists = mysql_num_rows(result.get()) > 0U;
```

**`mysql_num_rows(...)`**：获取该结果集中的**行数**（返回值类型是 `my_ulonglong`，即 `unsigned long long`）。

-   **`> 0U`**：与无符号整数 0 进行比较。因为 `mysql_num_rows` 返回的是无符号类型，用 `0U`（`unsigned int` 类型的 0）进行匹配，可以避免某些编译器关于“有符号/无符号比较”的警告，并保持类型的纯正性。
-   **赋值给 `exists`**：最终得到一个 `bool` 值。`1 > 0` 为 `true`，`0 > 0` 为 `false`。

```c
unsigned long* lengths = mysql_fetch_lengths(result.get());
```

**它告诉你刚才取出的那行数据里，每一列到底有多长（按字节数计）**

`mysql_fetch_row()` 返回的是 `char**`（指向 C 风格字符串的指针）。很多人会下意识地用 `strlen(row[0])` 来获取长度，但这是**严重错误**的，原因有二：

-   **二进制数据（BLOB / BINARY）**：这类字段可能包含 `\0`（空字节）。`strlen` 遇到 `\0` 就会停止计数，导致你读到的长度比实际数据短，从而截断用户的头像或加密消息。
-   **数据完整性校验**：MySQL 结果集传输时，字段长度是在协议包中明确告知客户端的。直接使用 `mysql_fetch_lengths` 可以**无需遍历字符串**（O(1) 操作）直接拿到精确长度，性能更高。

```c
result_value = mysql_num_rows(result.get()) > 0U;
```

**将 MySQL 结果集的行数（`my_ulonglong`）转换为 C++ 的布尔值（`bool`）**，用于判断 **“查询是否返回了至少一行数据”**。

```c
removed = mysql_affected_rows(connection_) == 1U;
```

**检查上一条 SQL 语句（通常是 `DELETE`、`UPDATE` 或 `INSERT ... ON DUPLICATE KEY UPDATE`）是否**恰好**影响了 1 行数据**，并将这个判断结果（`true`/`false`）赋值给 `removed` 变量。

```c
bool MySqlDatabase::pending_private_messages(
    const std::string& recipient,
    std::size_t count,
    std::vector<StoredMessage>& messages,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT m.id,m.payload "
        "FROM private_message_deliveries d "
        "JOIN messages m ON m.id=d.message_id "
        "WHERE d.recipient_username='" +
        escape(recipient) +
        "' AND d.delivered_at_unix_ms IS NULL "
        "ORDER BY m.id ASC LIMIT " +
        std::to_string(count);

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    messages.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());

        StoredMessage message;
        message.id = std::strtoull(row[0], nullptr, 10);

        const std::string bytes(row[1], lengths[1]);
        if (!parse_chat_message(bytes, message.payload)) {
            error = "failed to decode an offline private message";
            return false;
        }

        messages.push_back(std::move(message));
    }

    return true;
}

```

**为指定用户（`recipient`）从数据库中取出尚未投递（未标记已读）的离线私信，并按发送顺序（ID 升序）限制条数返回。**

它是聊天服务器中**“用户上线或刷新时收取未读消息”**功能的具体实现。

###### 1. 函数的核心流程（按步骤拆解）

-   **输入**：接收者用户名 `recipient`，最大拉取条数 `count`。
-   **输出**：填充 `messages` 向量（存储消息 ID 和解析后的内容），并返回 `bool` 表示操作成败。

**SQL 语句核心逻辑**（这是理解业务的关键）：



```sql
SELECT m.id, m.payload
FROM private_message_deliveries d
JOIN messages m ON m.id = d.message_id
WHERE d.recipient_username='xxx' AND d.delivered_at_unix_ms IS NULL
ORDER BY m.id ASC LIMIT 10
```



-   **`private_message_deliveries` 表**：这是一个“收件箱/投递队列”表。`delivered_at_unix_ms IS NULL` 表示**该消息尚未被接收方客户端拉取确认**（即“未读”或“离线”状态）。
-   **JOIN `messages` 表**：获取消息的实际内容（`payload`），通常包含发送者、时间戳、消息类型和正文。
-   **ORDER BY m.id ASC**：按消息 ID 升序，确保最早的消息先被拉取（符合聊天阅读习惯）。
-   **LIMIT count**：分批拉取，防止一个用户积压了 10 万条消息导致一次查询拖垮数据库。

------

###### 2. 数据解析细节（你的代码精妙之处）

在 `while` 循环中解析每一行时：



```cpp
message.id = std::strtoull(row[0], nullptr, 10);
const std::string bytes(row[1], lengths[1]);
if (!parse_chat_message(bytes, message.payload)) { ... }
```



-   **`std::strtoull`**：将 `id` 字段（字符串形式）转为 `unsigned long long`。你选择这种 C 风格函数而非 `std::stoull`，可能是为了避免异常抛出（因为这里无法处理异常）。
-   **`std::string(row[1], lengths[1])`**：利用 `mysql_fetch_lengths` 精确构造 `payload` 二进制内容。这里极其稳健，因为消息内容可能包含 `\0` 或非 UTF-8 的二进制头（如果协议里嵌了长度前缀）。
-   **`parse_chat_message`**：将数据库里的原始字节流反序列化为你的业务结构体（`ChatMessage` 或类似），解耦了存储层和协议层。

------

###### 3. 架构设计意图（为什么分成两张表）

你这里用了 `messages`（内容表）和 `private_message_deliveries`（投递表）两张表，这是经典的 **“内容与投递状态分离”** 设计：

-   **`messages`**：只存一份消息内容（`payload`），避免群发时重复存储巨大文本。
-   **`deliveries`**：记录每个接收者的投递状态（`delivered_at_unix_ms`）。如果发送给 100 个人，`messages` 表只插 1 条，`deliveries` 表插 100 条（分别标记已读/未读）。

这在大规模聊天室中极大地节省了存储空间和写入 I/O。





### password

```c#
//将二进制数据转换称十六进制字符串
std::string to_hex(const unsigned char* data, std::size_t size) {
 std::ostringstream output;                // 创建字符串流，充当“缓冲区”
 output << std::hex << std::setfill('0');  // 设置格式：十六进制模式，不足位补 '0'

 for (std::size_t i = 0; i < size; ++i) {
    output << std::setw(2) << static_cast<unsigned int>(data[i]); 
    // setw(2)：告诉流“下一个数字至少占 2 个字符宽度”
    // 强制转 unsigned int：防止 char 为负数时，<< 输出一堆 'f' 前缀（符号扩展）
 }
 return output.str(); // 将流内容提取为 std::string
}
```

-   **密码哈希存储（`password.cpp`）**：
    用户注册或登录时，OpenSSL 的 `SHA256` 或 `PBKDF2` 函数会生成二进制摘要（例如 32 字节的 `unsigned char` 数组）。数据库（MySQL）通常无法直接存储二进制乱码，所以必须调用 `to_hex()` 将其转为可读的 `"5e8848..."` 字符串存入 `VARCHAR` 字段。
-   **网络协议调试（日志/抓包）**：
    在 `src/protocol.cpp` 或 `TcpConnection` 的 `onMessage` 中，为了排查客户端发送的非法乱码，工程师会打印 `to_hex(data, len)` 到日志文件，方便肉眼查看网络字节流的原始模样

```c#
bool from_hex(const std::string& text, std::vector<unsigned char>& output) {
    if (text.size() % 2 != 0) return false;  // 十六进制必须是偶数长度
     output.clear();
      output.reserve(text.size() / 2);          // 预分配内存，避免动态扩容

for (std::size_t i = 0; i < text.size(); i += 2) {
    unsigned int value = 0;
    // 危险操作：每次截取 2 个字符（如 "1a"）生成临时字符串
    std::istringstream input(text.substr(i, 2)); 
    input >> std::hex >> value;           // 以十六进制方式读取，存入 value
    if (!input || value > 255) return false;
    output.push_back(static_cast<unsigned char>(value));
}
}
```

-   每次循环从原字符串中取 2 个字符（如 `"1a"`），单独构造一个 `std::string` 子串。
-   用 `std::istringstream` 解析这个子串为十六进制数。
-   校验解析成功且值不溢出（`value > 255` 实际上极少触发，因为两位十六进制最大就是 255）。

### proto_codec

```c
void append_varint(std::string& out, std::uint64_t value) {
    while (value >= 0x80U) {  // 0x80 = 128 (二进制 1000 0000)
    // 1. 取出低 7 位 (value & 0x7F)
    // 2. 强制把最高位设为 1 (| 0x80)，告诉解码器"后面还有字节"
    out.push_back(static_cast<char>((value & 0x7FU) | 0x80U));
    value >>= 7U; // 右移 7 位，继续处理高位的部分
}
// 最后一个字节：最高位为 0，表示"这是结尾"，直接存入
out.push_back(static_cast<char>(value));
}
```

**举例（数字 300 的编码过程）**：

-   `300` 的二进制是 `1 0010 1100`（需要 9 位存储）。
-   **循环第 1 次**：取低 7 位 `010 1100` (44)，最高位置 1 得 `1010 1100` (0xAC)，存入。`value` 右移变为 `2`。
-   **循环结束**：`2 < 128`，直接存入 `0x02`。
-   **最终结果**：`0xAC 0x02`（十六进制）。解码时读 0xAC（首位为1，继续读），读 0x02（首位为0，停止），计算 `(0xAC & 0x7F) + (0x02 << 7) = 44 + 256 = 300`。

在你的 `minimuduo` 自定义协议中，这个函数通常用于组装发送包（`send` 缓冲区）：

-   **消息长度（Length）**：如果用固定 4 字节存长度，心跳包（内容为空）就会浪费 4 字节。用 Varint 存长度，心跳包长度 `0` 只占 **1 个字节**（`0x00`）。

-   **命令枚举（Cmd）**：登录、登出、私聊等命令编号通常很小（如 `1`），Varint 只需 1 字节。

-   **用户 ID**：虽然 ID 可能很大，但在大多数活跃连接数不多时，ID 通常小于 128，同样只需 1 字节。

    

    

```c#
void append_string_field(
    std::string& out,
    std::uint32_t field,
    const std::string& value
) {
    if (value.empty()) {
        return;
    }

    append_tag(out, field, 2U);
    append_varint(out, value.size());
    out.append(value);
}
```

**将一个字符串字段，按照 Protobuf 风格的 "Tag + Length + Value"（键-长度-值）格式，写入输出缓冲区。如果字符串为空，则直接跳过，不占任何字节**

-   `wire_type = 2` 代表 **LENGTH_DELIMITED**（长度分隔类型），告诉解码器：后面跟的是一个长度值，接着是这么长的原始字节。

-   

-   | 操作                  | 生成的数据（十六进制）     | 说明                                      |
    | :-------------------- | :------------------------- | :---------------------------------------- |
    | `append_tag`          | `0x0A`                     | `(1 << 3) | 2 = 10`，Varint 编码为 `0x0A` |
    | `append_varint`       | `0x05`                     | 字符串长度 5，Varint 编码为 `0x05`        |
    | `out.append("Alice")` | `0x41 0x6C 0x69 0x63 0x65` | ASCII 码                                  |
    | **最终包片段**        | `0A 05 41 6C 69 63 65`     | 总共 7 个字节                             |

### 	EventLoop

```c++
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_.store(true);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor& functor : functors) {
        functor();
    }

    callingPendingFunctors_.store(false);
}
```

-   **如果不加 `swap`**：你可能会想“直接遍历 `pendingFunctors_` 不就行了？”但不行，因为遍历期间必须一直锁着 `mutex_`。如果这些函数执行过程中，又有别的线程往里 `push` 新任务，就会发生**锁冲突**，甚至死锁。
-   **用了 `swap` 的妙处**：它只在**极短的时间**（微秒级）内持有锁，把队列里的任务**“整个偷走”**（交换到局部变量 `functors`），然后**立刻释放锁**。这样一来，其他线程（比如主线程）可以随时继续往空的 `pendingFunctors_` 里无阻塞地添加新任务，而 I/O 线程则在锁外面，安心、安全地慢慢执行手中的旧任务列表。
-   **关键点**：`isInLoopThread()` 判断当前线程是不是 I/O 线程。
    -   当 `doPendingFunctors` 正在执行 `functor()` 时，如果这个 `functor` 里又调用了 `queueInLoop`，且 `callingPendingFunctors_` 为 `true`，库通常会**不唤醒** `eventfd`（因为 I/O 线程正在活跃运行，不需要额外唤醒），避免了无效的系统调用。





##### 设计

-   **MySQL（服务端全局数据库）**：存所有用户的账号、好友关系、群组、以及**历史的聊天消息**。所有 `ChatServer` 实例（如果你以后扩容）都得连同一个 MySQL，保证数据一致。
-   **Redis（服务端内存缓存）**：存用户的**在线状态**（哪个服务器实例负责）和**未读消息数**。这东西必须快，且必须跨进程共享，所以也得是独立的服务。
-   **SQLite（客户端本地缓存）**：这是在 **`client.cpp`（命令行客户端）** 里用的。它只存**你自己这台电脑**上的聊天记录，方便你离线查看历史。因为它只属于你一个人，没必要启动一个庞大的数据库服务，直接存成文件最简单。

### sqlite_client

好的，我来逐行讲解这段 `SqliteClient` 的 C++ 代码。它封装了 SQLite 数据库操作，用于缓存私聊消息、群聊消息和文件传输记录。我会按函数和逻辑块展开，说明每行（或每段）在做什么。

---

```cpp
namespace {
```

- 开始一个**匿名命名空间**，其中的类型和函数仅在当前编译单元可见，避免符号冲突。

---

#### `StatementPtr` 类型别名

```cpp
using StatementPtr =
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
```

- 定义 `StatementPtr` 为 **智能指针**，管理 `sqlite3_stmt*`（预处理语句对象），删除器是 `sqlite3_finalize`。这样当指针销毁时会自动调用 `sqlite3_finalize` 释放语句资源，实现 RAII。

---

#### `bind_text` 辅助函数

```cpp
bool bind_text(
    sqlite3_stmt* statement,
    int index,
    const std::string& value
) {
    return sqlite3_bind_text(
        statement,//预编译语句对象
        index,//占位符的索引
        value.c_str(),//要绑定到占位符的数据指针
        static_cast<int>(value.size()),//要绑定数据的长度
        SQLITE_TRANSIENT//为了确保指针不会被一位修改，在内部复制字符串保存，保证指针有效
    ) == SQLITE_OK;
}
```

- 封装 `sqlite3_bind_text`，将 `std::string` 绑定到 SQL 语句的参数占位符（`?`）上。
- `SQLITE_TRANSIENT` 告诉 SQLite “**这块内存可能随时失效（因为是临时 string），请你在内部把数据拷贝一份。**”
- 返回绑定是否成功（`SQLITE_OK`）。

---

#### `sqlite_error` 辅助函数

```cpp
std::string sqlite_error(sqlite3* database) {
    return database != nullptr
        ? sqlite3_errmsg(database)
        : "SQLite database is null";
}
```

- 从数据库连接对象获取最近一次错误的描述字符串；若连接为空则返回自定义信息。

---

匿名命名空间结束。

---

#### 2. 析构函数 `~SqliteClient()`

```cpp
SqliteClient::~SqliteClient() {
    std::lock_guard<std::mutex> lock(mutex_);
    close_locked();
}
```

- 析构时加锁（保护多线程环境），然后调用 `close_locked()` 关闭数据库连接。确保资源释放。

---

#### 3. 打开数据库 `open`

```cpp
bool SqliteClient::open(
    const std::string& database_path,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(mutex_);
```

- 加互斥锁，保证线程安全。

```cpp
    close_locked();
```

- 如果已经打开，先关闭旧的连接（避免重复打开）。

```cpp
    const std::filesystem::path path(database_path);
    if (path.has_parent_path() &&
        !path.parent_path().empty()) {
        std::error_code directory_error;
        std::filesystem::create_directories(
            path.parent_path(),
            directory_error
        );

        if (directory_error) {
            error =
                "cannot create SQLite directory: " +
                directory_error.message();
            return false;
        }
    }
```

- 检查路径是否包含父目录，如果有则递归创建目录（`create_directories`），若失败则返回错误。**在创建或打开SQLite数据库文件之前，先确保存放它的父目录（文件夹）已经存在。如果不存在，就递归地创建出来。**

```cpp
    if (sqlite3_open_v2(
            database_path.c_str(),
            &database_,
            SQLITE_OPEN_READWRITE |
                SQLITE_OPEN_CREATE |
                SQLITE_OPEN_FULLMUTEX,
            nullptr
        ) != SQLITE_OK) {
        error = sqlite_error(database_);
        close_locked();
        return false;
    }
```

- 用 `sqlite3_open_v2` 打开或创建数据库文件。
- 标志：`READWRITE | CREATE` 表示读写且不存在则创建；`FULLMUTEX` 启用线程安全模式（串行化）。
- 若失败，获取错误信息，关闭连接并返回 false。

```cpp
    database_path_ = database_path;
    sqlite3_busy_timeout(database_, 3000);
```

- 保存路径；设置忙超时为 3000 毫秒（3秒），当数据库被锁定时等待这么长时间再返回错误。

```cpp
    return initialize_schema(error);
}
```

- 调用 `initialize_schema` 创建表（如果不存在），返回其结果。

---

#### 4. 缓存私聊消息 `cache_private_message`

```cpp
bool SqliteClient::cache_private_message(
    const LocalPrivateMessage& message,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(mutex_);
```

- 加锁。

```cpp
    static constexpr const char* sql =
        "INSERT INTO private_messages("
        "account_username,server_message_id,peer_username,"
        "sender_username,recipient_username,content,"
        "received_at_unix_ms,outgoing,offline_delivery"
        ") VALUES(?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(account_username,server_message_id) "
        "DO UPDATE SET "
        "peer_username=excluded.peer_username,"
        "sender_username=excluded.sender_username,"
        "recipient_username=excluded.recipient_username,"
        "content=excluded.content,"
        "received_at_unix_ms=MIN("
        "private_messages.received_at_unix_ms,"
        "excluded.received_at_unix_ms),"
        "outgoing=excluded.outgoing,"
        "offline_delivery=MAX("
        "private_messages.offline_delivery,"
        "excluded.offline_delivery)";
```

- SQL 插入语句，带有 `ON CONFLICT` 处理：当 `(account_username, server_message_id)` 冲突时，执行更新。**保证同一条服务端消息，不会在某个用户的收件箱里重复插入两次。**

- 比如服务器发送消息后，客户端因没收到 ACK 而重连请求，服务器再次尝试插入同一条 `server_message_id`。此时不会报错，而是触发更新。

- 更新时：`received_at_unix_ms` 取旧值和新值中的较小值（保留最早时间），`offline_delivery` 取最大值（更可能为离线），其他字段用新值覆盖。

- 

- | 参数                  | 物理约束     | 值域约束      | 业务逻辑约束（与其他字段的关系）              |
    | :-------------------- | :----------- | :------------ | :-------------------------------------------- |
    | `account_username`    | NOT NULL, PK | 存在的用户    | 定义为当前收件箱/发件箱的主人                 |
    | `server_message_id`   | NOT NULL, PK | 全局唯一 ID   | 与另一个 account 的同 ID 记录配对             |
    | `peer_username`       | NOT NULL     | 存在的用户    | 不能等于 `account_username`                   |
    | `sender_username`     | NOT NULL     | 存在的用户    | 与 `outgoing` 值联动（见约束 A）              |
    | `recipient_username`  | NOT NULL     | 存在的用户    | 与 `outgoing` 值联动（见约束 A）              |
    | `content`             | NOT NULL     | 文本/长度限制 | 配对的两条记录必须完全相同                    |
    | `received_at_unix_ms` | NOT NULL     | 正毫秒时间戳  | 冲突更新时只能取 MIN（变小）                  |
    | `outgoing`            | NOT NULL     | 0 或 1        | 决定了 `account` 与 `sender/recipient` 的关系 |
    | `offline_delivery`    | NOT NULL     | 0 或 1        | 冲突更新时只能取 MAX（变成 1）                |

```cpp
    sqlite3_stmt* raw_statement = nullptr;

    if (sqlite3_prepare_v2(
            database_,
            sql,
            -1,
            &raw_statement,
            nullptr
        ) != SQLITE_OK) {
        error = sqlite_error(database_);
        return false;
    }
```

- 准备 SQL 语句，`-1` 表示读取完整字符串。若失败则返回错误。

```cpp
    StatementPtr statement(
        raw_statement,
        sqlite3_finalize
    );
```

- 将原始指针包装进 `StatementPtr`，确保自动 finalize。

```cpp
    const bool bound =
        bind_text(statement.get(), 1, message.account_username) &&
        sqlite3_bind_int64(
            statement.get(),
            2,
            static_cast<sqlite3_int64>(
                message.server_message_id
            )
        ) == SQLITE_OK &&
        bind_text(statement.get(), 3, message.peer_username) &&
        bind_text(statement.get(), 4, message.sender_username) &&
        bind_text(statement.get(), 5, message.recipient_username) &&
        bind_text(statement.get(), 6, message.content) &&
        sqlite3_bind_int64(
            statement.get(),
            7,
            static_cast<sqlite3_int64>(
                message.received_at_unix_ms
            )
        ) == SQLITE_OK &&
        sqlite3_bind_int(
            statement.get(),
            8,
            message.outgoing ? 1 : 0
        ) == SQLITE_OK &&
        sqlite3_bind_int(
            statement.get(),
            9,
            message.offline_delivery ? 1 : 0
        ) == SQLITE_OK;
```

- 按顺序绑定所有参数（索引从1开始）。`bind_text` 绑定字符串，`sqlite3_bind_int64` 绑定整数，`sqlite3_bind_int` 绑定布尔值（转为0/1）。
- `bound` 为 `true` 表示所有绑定成功。

```cpp
    if (!bound) {
        error = sqlite_error(database_);
        return false;
    }
```

- 若绑定失败，返回错误。

```cpp
    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        error = sqlite_error(database_);
        return false;
    }

    return true;
}
```

- 执行语句，若结果不是 `SQLITE_DONE`（表示完成插入/更新），则报错；否则成功。

---

#### 5. 缓存群聊消息 `cache_group_message`

与 `cache_private_message` 结构几乎相同，但针对 `group_messages` 表，字段少了 `recipient_username`（群聊不需要接收者），其他类似。不再赘述每行，逻辑一致。

---

#### 6. 获取最近私聊消息 `recent_private_messages`

```cpp
bool SqliteClient::recent_private_messages(
    const std::string& account_username,
    const std::string& peer_username,
    std::size_t count,
    std::vector<LocalPrivateMessage>& messages,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(mutex_);
```

- 加锁。

```cpp
    static constexpr const char* sql =
        "SELECT server_message_id,peer_username,"
        "sender_username,recipient_username,content,"
        "received_at_unix_ms,outgoing,offline_delivery "
        "FROM private_messages "
        "WHERE account_username=? AND peer_username=? "
        "ORDER BY server_message_id DESC LIMIT ?";
```

- 查询指定账户和对方的最近 `count` 条消息，按 `server_message_id` 降序（最新的在前）取前 `count` 条。

```cpp
    sqlite3_stmt* raw_statement = nullptr;
    if (sqlite3_prepare_v2(...) != SQLITE_OK) { ... }
    StatementPtr statement(...);
```

- 准备并包装语句。

```cpp
    if (!bind_text(statement.get(), 1, account_username) ||
        !bind_text(statement.get(), 2, peer_username) ||
        sqlite3_bind_int64(
            statement.get(),
            3,
            static_cast<sqlite3_int64>(count)
        ) != SQLITE_OK) {
        error = sqlite_error(database_);
        return false;
    }
```

- 绑定三个参数：用户名、对方用户名、数量。

```cpp
    messages.clear();
```

- 清空输出向量。

```cpp
    while (true) {
        const int result = sqlite3_step(statement.get());

        if (result == SQLITE_DONE) {
            break;
        }

        if (result != SQLITE_ROW) {
            error = sqlite_error(database_);
            return false;
        }
```

- 循环取行：`SQLITE_DONE` 表示结束，`SQLITE_ROW` 表示有数据，其他为错误。

```cpp
        LocalPrivateMessage message;
        message.server_message_id =
            static_cast<std::uint64_t>(
                sqlite3_column_int64(
                    statement.get(),
                    0
                )
            );
        message.account_username = account_username;
        message.peer_username =
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement.get(), 1)
            );
        // ... 类似取出其他列
```

- 按列索引取出数据，`sqlite3_column_int64` 取整数，`sqlite3_column_text` 取字符串（返回 `const unsigned char*`，强转为 `const char*`）。
- `account_username` 直接用函数参数赋值。

```cpp
        messages.push_back(std::move(message));
    }
```

- 将消息对象加入向量（移动语义）。

```cpp
    std::reverse(messages.begin(), messages.end());
    return true;
}
```

- 因为查询是降序（最新在前），但我们希望返回的消息按时间升序（最旧在前），所以反转整个向量。

---

#### 7. 获取最近群聊消息 `recent_group_messages`

与上述类似，表为 `group_messages`，查询条件为 `account_username` 和 `group_name`，列少了 `recipient_username`。逻辑完全一致。

---

#### 8. 缓存文件传输记录 `cache_file_transfer`

```cpp
bool SqliteClient::cache_file_transfer(
    const LocalFileTransfer& file,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(mutex_);
```

- 加锁。

```cpp
    static constexpr const char* sql =
        "INSERT INTO file_transfers("
        "account_username,server_transfer_id,scope,"
        "peer_username,group_name,sender_username,"
        "file_name,local_path,file_size,sha256_hex,"
        "received_at_unix_ms,outgoing"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(account_username,server_transfer_id) "
        "DO UPDATE SET "
        "scope=excluded.scope,"
        "peer_username=excluded.peer_username,"
        "group_name=excluded.group_name,"
        "sender_username=excluded.sender_username,"
        "file_name=excluded.file_name,"
        "local_path=excluded.local_path,"
        "file_size=excluded.file_size,"
        "sha256_hex=excluded.sha256_hex,"
        "received_at_unix_ms=excluded.received_at_unix_ms,"
        "outgoing=excluded.outgoing";
```

- SQL 插入，冲突时更新所有字段（除了主键），不涉及 MIN/MAX 聚合，直接覆盖。

```cpp
    sqlite3_stmt* raw_statement = nullptr;
    if (sqlite3_prepare_v2(...) != SQLITE_OK) { ... }
    StatementPtr statement(...);
```

- 准备。

```cpp
    const bool bound =
        bind_text(statement.get(), 1, file.account_username) &&
        sqlite3_bind_int64(...) == SQLITE_OK &&  // server_transfer_id
        bind_text(statement.get(), 3, file.scope) &&
        bind_text(statement.get(), 4, file.peer_username) &&
        bind_text(statement.get(), 5, file.group_name) &&
        bind_text(statement.get(), 6, file.sender_username) &&
        bind_text(statement.get(), 7, file.file_name) &&
        bind_text(statement.get(), 8, file.local_path) &&
        sqlite3_bind_int64(...) == SQLITE_OK &&  // file_size
        bind_text(statement.get(), 10, file.sha256_hex) &&
        sqlite3_bind_int64(...) == SQLITE_OK &&  // received_at_unix_ms
        sqlite3_bind_int(...) == SQLITE_OK;      // outgoing
```

- 绑定所有参数，注意索引从1到12。

```cpp
    if (!bound) { error = sqlite_error(database_); return false; }
    if (sqlite3_step(statement.get()) != SQLITE_DONE) { ... }
    return true;
}
```

- 执行并检查。

---

#### 9. 获取最近文件传输记录 `recent_file_transfers`

```cpp
bool SqliteClient::recent_file_transfers(
    const std::string& account_username,
    std::size_t count,
    std::vector<LocalFileTransfer>& files,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(mutex_);
```

- 加锁。

```cpp
    static constexpr const char* sql =
        "SELECT server_transfer_id,scope,peer_username,"
        "group_name,sender_username,file_name,local_path,"
        "file_size,sha256_hex,received_at_unix_ms,outgoing "
        "FROM file_transfers "
        "WHERE account_username=? "
        "ORDER BY server_transfer_id DESC LIMIT ?";
```

- 查询某个账户最近的文件传输记录，按 `server_transfer_id` 降序。

```cpp
    // 准备、绑定
    if (!bind_text(...) || sqlite3_bind_int64(...) != SQLITE_OK) { ... }
```

- 绑定两个参数。

```cpp
    files.clear();
    while (true) {
        int result = sqlite3_step(statement.get());
        // 处理行
    }
    std::reverse(files.begin(), files.end());
    return true;
}
```

- 循环读取，填充 `LocalFileTransfer` 结构。注意这里使用了 `column_text` lambda 来处理可能为 NULL 的文本列（返回空字符串）。最后反转顺序。

---

#### 10. 统计信息 `stats`

```cpp
bool SqliteClient::stats(
    const std::string& account_username,
    LocalCacheStats& stats_value,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(mutex_);
```

- 加锁。

```cpp
    static constexpr const char* sql =
        "SELECT "
        "(SELECT COUNT(*) FROM private_messages "
        "WHERE account_username=?),"
        "(SELECT COUNT(*) FROM group_messages "
        "WHERE account_username=?),"
        "(SELECT COUNT(*) FROM file_transfers "
        "WHERE account_username=?)";
```

- 一条 SQL 返回三个子查询的结果，分别统计私聊、群聊、文件记录数。

```cpp
    sqlite3_stmt* raw_statement = nullptr;
    if (sqlite3_prepare_v2(...) != SQLITE_OK) { ... }
    StatementPtr statement(...);
```

- 准备。

```cpp
    if (!bind_text(statement.get(), 1, account_username) ||
        !bind_text(statement.get(), 2, account_username) ||
        !bind_text(statement.get(), 3, account_username)) {
        error = sqlite_error(database_);
        return false;
    }
```

- 三个占位符都绑定同一个用户名。

```cpp
    if (sqlite3_step(statement.get()) != SQLITE_ROW) {
        error = sqlite_error(database_);
        return false;
    }
```

- 执行，期望至少有一行结果（因为聚合查询总是返回一行）。

```cpp
    stats_value.private_messages =
        static_cast<std::size_t>(
            sqlite3_column_int64(statement.get(), 0)
        );
    // 类似取第1、2列
    return true;
}
```

- 取出三列整数值并赋值给输出结构。

---

#### 11. 执行任意 SQL `execute`

```cpp
bool SqliteClient::execute(
    const std::string& sql,
    std::string& error
) {
    char* raw_error = nullptr;

    const int result = sqlite3_exec(
        database_,
        sql.c_str(),
        nullptr,
        nullptr,
        &raw_error
    );

    if (result == SQLITE_OK) {
        return true;
    }

    error =
        raw_error != nullptr
            ? raw_error
            : sqlite_error(database_);

    sqlite3_free(raw_error);
    return false;
}
```

- 使用 `sqlite3_exec` 执行非查询 SQL（如创建表、PRAGMA 等）。
- `raw_error` 接收错误信息（如果有），成功时 `sqlite3_exec` 返回 `SQLITE_OK`，否则构造错误字符串并释放 `raw_error`。

---

#### 12. 初始化表结构 `initialize_schema`

```cpp
bool SqliteClient::initialize_schema(
    std::string& error
) {
    static constexpr const char* schema = R"SQL(
PRAGMA journal_mode=WAL;
PRAGMA synchronous=NORMAL;
PRAGMA foreign_keys=ON;

CREATE TABLE IF NOT EXISTS private_messages (
    account_username TEXT NOT NULL,
    server_message_id INTEGER NOT NULL,
    peer_username TEXT NOT NULL,
    sender_username TEXT NOT NULL,
    recipient_username TEXT NOT NULL,
    content TEXT NOT NULL,
    received_at_unix_ms INTEGER NOT NULL,
    outgoing INTEGER NOT NULL CHECK (outgoing IN (0,1)),
    offline_delivery INTEGER NOT NULL
        CHECK (offline_delivery IN (0,1)),
    PRIMARY KEY (account_username, server_message_id)
);

CREATE INDEX IF NOT EXISTS
idx_local_private_peer
ON private_messages(
    account_username,
    peer_username,
    server_message_id
);

-- 类似定义 group_messages 和 file_transfers 表及索引
)SQL";

    return execute(schema, error);
}
```

- 使用原始字符串字面量 `R"SQL(...)SQL"` 嵌入多行 SQL。
- 设置 PRAGMA：`journal_mode=WAL` 启用预写日志提高并发；`synchronous=NORMAL` 平衡性能与安全；`foreign_keys=ON` 启用外键约束（虽然本表未用）。
- 创建三个表，每个表都有主键和检查约束，并创建索引以优化查询。
- 最后调用 `execute` 执行这些 SQL。

---

#### 13. 关闭数据库（内部）`close_locked`

```cpp
void SqliteClient::close_locked() {
    if (database_ != nullptr) {
        sqlite3_close(database_);
        database_ = nullptr;
    }

    database_path_.clear();
}
```

- 前提：调用时已持有锁（由 `open` 和析构函数确保）。
- 若数据库连接非空，调用 `sqlite3_close` 关闭，并置空指针。
- 清空保存的路径字符串。

---

#### 📦 新增内容概览



- **两个数据表**：`pending_uploads`（待上传文件队列）和 `partial_downloads`（未完成的下载任务）
- **六个成员函数**：
  - `save_pending_upload` — 保存一条待上传记录
  - `list_pending_uploads` — 列出某账号的所有待上传记录
  - `remove_pending_upload` — 删除指定的待上传记录
  - `save_partial_download` — 保存一个未完成的下载任务
  - `get_partial_download` — 根据传输 ID 获取某个下载任务
  - `remove_partial_download` — 删除指定的下载任务

这些新增功能支持**断点续传**和**离线文件上传队列**的管理。

---

#### 🗃️ 新增表结构（在 `initialize_schema` 中）

新增的两张表定义如下（位于 `initialize_schema` 末尾）：

#### 1. `pending_uploads` 表

```sql
CREATE TABLE IF NOT EXISTS pending_uploads (
    account_username TEXT NOT NULL,
    transfer_token TEXT NOT NULL,
    scope TEXT NOT NULL,
    target TEXT NOT NULL,
    source_path TEXT NOT NULL,
    file_name TEXT NOT NULL,
    file_size INTEGER NOT NULL,
    sha256_hex TEXT NOT NULL,
    created_at_unix_ms INTEGER NOT NULL,
    PRIMARY KEY (account_username, transfer_token)
);
```

- **account_username**：所属账户
- **transfer_token**：服务端返回的传输令牌（唯一标识一次上传）
- **scope**：传输范围（如 `"private"` 或 `"group"`）
- **target**：目标（私聊时为对方用户名，群聊时为群组名）
- **source_path**：本地待上传文件的绝对路径
- **file_name**：原始文件名
- **file_size**：文件大小（字节）
- **sha256_hex**：文件的 SHA-256 哈希（用于去重和校验）
- **created_at_unix_ms**：任务创建时间（毫秒时间戳）
- **主键**：`(account_username, transfer_token)` 确保唯一

同时创建了索引：

```sql
CREATE INDEX IF NOT EXISTS
idx_pending_uploads_account
ON pending_uploads(
    account_username,
    created_at_unix_ms,
    transfer_token
);
```

该索引用于加速按账户查询，并按创建时间排序。

---

#### 2. `partial_downloads` 表

```sql
CREATE TABLE IF NOT EXISTS partial_downloads (
    account_username TEXT NOT NULL,
    server_transfer_id INTEGER NOT NULL,
    scope TEXT NOT NULL,
    sender_username TEXT NOT NULL,
    group_name TEXT NOT NULL DEFAULT '',
    file_name TEXT NOT NULL,
    temp_path TEXT NOT NULL,
    file_size INTEGER NOT NULL,
    sha256_hex TEXT NOT NULL,
    PRIMARY KEY (account_username, server_transfer_id)
);
```

- **account_username**：所属账户
- **server_transfer_id**：服务端传输 ID（唯一标识一次下载）
- **scope**：传输范围
- **sender_username**：发送者
- **group_name**：群组名（私聊时为空字符串）
- **file_name**：文件名
- **temp_path**：本地临时文件路径（已下载的部分内容）
- **file_size**：文件总大小
- **sha256_hex**：文件哈希
- **主键**：`(account_username, server_transfer_id)`

该表没有额外索引，但主键已能高效查询。



---

#### 1. `save_pending_upload`

**功能**：插入或更新一条待上传记录（`UPSERT`）。

```cpp
bool SqliteClient::save_pending_upload(
    const LocalPendingUpload& upload,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(
        mutex_
    );
```
- 加互斥锁，确保线程安全。

```cpp
    static constexpr const char* sql =
        "INSERT INTO pending_uploads("
        "account_username,transfer_token,scope,target,"
        "source_path,file_name,file_size,sha256_hex,"
        "created_at_unix_ms"
        ") VALUES(?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(account_username,transfer_token) "
        "DO UPDATE SET "
        "scope=excluded.scope,"
        "target=excluded.target,"
        "source_path=excluded.source_path,"
        "file_name=excluded.file_name,"
        "file_size=excluded.file_size,"
        "sha256_hex=excluded.sha256_hex,"
        "created_at_unix_ms=excluded.created_at_unix_ms";
```
- 定义 SQL 语句，9 个占位符。
- 当主键冲突时（同一账户、同一 `transfer_token` 已存在），则更新所有字段（除主键外）为新值，即覆盖旧的记录。

```cpp
    sqlite3_stmt* raw_statement =
        nullptr;

    if (sqlite3_prepare_v2(
            database_,
            sql,
            -1,
            &raw_statement,
            nullptr
        ) != SQLITE_OK) {
        error =
            sqlite_error(database_);
        return false;
    }
```
- 准备 SQL 语句，若失败则获取错误并返回。

```cpp
    StatementPtr statement(
        raw_statement,
        sqlite3_finalize
    );
```
- 用智能指针包装，确保自动 `sqlite3_finalize`。

```cpp
    const bool bound =
        bind_text(
            statement.get(),
            1,
            upload.account_username
        ) &&
        bind_text(
            statement.get(),
            2,
            upload.transfer_token
        ) &&
        bind_text(
            statement.get(),
            3,
            upload.scope
        ) &&
        bind_text(
            statement.get(),
            4,
            upload.target
        ) &&
        bind_text(
            statement.get(),
            5,
            upload.source_path
        ) &&
        bind_text(
            statement.get(),
            6,
            upload.file_name
        ) &&
        sqlite3_bind_int64(
            statement.get(),
            7,
            static_cast<sqlite3_int64>(
                upload.file_size
            )
        ) == SQLITE_OK &&
        bind_text(
            statement.get(),
            8,
            upload.sha256_hex
        ) &&
        sqlite3_bind_int64(
            statement.get(),
            9,
            static_cast<sqlite3_int64>(
                upload.created_at_unix_ms
            )
        ) == SQLITE_OK;
```
- 依次绑定所有参数，索引 1~9：
  - 1~6 是字符串，使用 `bind_text`
  - 7 是 `file_size`（无符号整型，转为 `sqlite3_int64`）
  - 8 是 SHA256 字符串
  - 9 是创建时间（`int64_t`）
- `bound` 为 `true` 表示全部绑定成功。

```cpp
    if (!bound) {
        error =
            sqlite_error(database_);
        return false;
    }

    if (sqlite3_step(
            statement.get()
        ) != SQLITE_DONE) {
        error =
            sqlite_error(database_);
        return false;
    }

    return true;
}
```
- 若绑定失败，取错误信息返回。
- 执行语句，若结果不是 `SQLITE_DONE` 则报错，否则成功返回。

---

#### 2. `list_pending_uploads`

**功能**：列出某账户的所有待上传任务（按创建时间升序，令牌作为次排序）。

```cpp
bool SqliteClient::list_pending_uploads(
    const std::string& account_username,
    std::vector<LocalPendingUpload>& uploads,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(
        mutex_
    );
```
- 加锁。

```cpp
    static constexpr const char* sql =
        "SELECT transfer_token,scope,target,source_path,"
        "file_name,file_size,sha256_hex,created_at_unix_ms "
        "FROM pending_uploads "
        "WHERE account_username=? "
        "ORDER BY created_at_unix_ms,transfer_token";
```
- 查询所有字段（除了 `account_username`，因为查询条件已知），并按创建时间升序、令牌升序排列，保证稳定的输出顺序。

```cpp
    sqlite3_stmt* raw_statement =
        nullptr;

    if (sqlite3_prepare_v2(
            database_,
            sql,
            -1,
            &raw_statement,
            nullptr
        ) != SQLITE_OK) {
        error =
            sqlite_error(database_);
        return false;
    }

    StatementPtr statement(
        raw_statement,
        sqlite3_finalize
    );
```
- 准备语句并包装。

```cpp
    if (!bind_text(
            statement.get(),
            1,
            account_username
        )) {
        error =
            sqlite_error(database_);
        return false;
    }
```
- 绑定账户名（唯一参数）。

```cpp
    uploads.clear();
```
- 清空输出向量，准备填入新数据。

```cpp
    auto column_text =
        [&](int column) {
            const unsigned char* value =
                sqlite3_column_text(
                    statement.get(),
                    column
                );

            return value == nullptr
                ? std::string()
                : std::string(
                      reinterpret_cast<
                          const char*
                      >(value)
                  );
        };
```
- 定义一个 lambda，用于安全地从 `sqlite3_column_text` 获取字符串，若为 `NULL` 则返回空字符串（`pending_uploads` 所有字段均为 `NOT NULL`，但此处通用处理）。

```cpp
    while (true) {
        const int result =
            sqlite3_step(
                statement.get()
            );

        if (result == SQLITE_DONE) {
            break;
        }

        if (result != SQLITE_ROW) {
            error =
                sqlite_error(database_);
            return false;
        }
```
- 循环取行，直到结束或出错。

```cpp
        LocalPendingUpload upload;
        upload.account_username =
            account_username;
        upload.transfer_token =
            column_text(0);
        upload.scope =
            column_text(1);
        upload.target =
            column_text(2);
        upload.source_path =
            column_text(3);
        upload.file_name =
            column_text(4);
        upload.file_size =
            static_cast<std::uint64_t>(
                sqlite3_column_int64(
                    statement.get(),
                    5
                )
            );
        upload.sha256_hex =
            column_text(6);
        upload.created_at_unix_ms =
            static_cast<std::int64_t>(
                sqlite3_column_int64(
                    statement.get(),
                    7
                )
            );

        uploads.push_back(
            std::move(upload)
        );
    }
```
- 从列索引 0~7 依次取出各字段值，填充 `LocalPendingUpload` 对象，然后移动加入向量。

```cpp
    return true;
}
```
- 成功返回。

---

#### 3. `remove_pending_upload`

**功能**：根据账户和传输令牌删除一条待上传记录。

```cpp
bool SqliteClient::remove_pending_upload(
    const std::string& account_username,
    const std::string& transfer_token,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(
        mutex_
    );
```
- 加锁。

```cpp
    static constexpr const char* sql =
        "DELETE FROM pending_uploads "
        "WHERE account_username=? "
        "AND transfer_token=?";
```
- DELETE 语句，两个条件。

```cpp
    sqlite3_stmt* raw_statement =
        nullptr;

    if (sqlite3_prepare_v2(
            database_,
            sql,
            -1,
            &raw_statement,
            nullptr
        ) != SQLITE_OK) {
        error =
            sqlite_error(database_);
        return false;
    }

    StatementPtr statement(
        raw_statement,
        sqlite3_finalize
    );
```
- 准备语句并包装。

```cpp
    if (!bind_text(
            statement.get(),
            1,
            account_username
        ) ||
        !bind_text(
            statement.get(),
            2,
            transfer_token
        )) {
        error =
            sqlite_error(database_);
        return false;
    }
```
- 绑定两个参数。

```cpp
    if (sqlite3_step(
            statement.get()
        ) != SQLITE_DONE) {
        error =
            sqlite_error(database_);
        return false;
    }

    return true;
}
```
- 执行，若成功则返回 true（即使没有删除任何行，SQLite 也会返回 `SQLITE_DONE`，因为 DELETE 操作成功，受影响行数可能为 0，但这不是错误，所以调用者不关心行数）。

---

#### 4. `save_partial_download`

**功能**：保存一个未完成的下载任务（断点续传信息）。

```cpp
bool SqliteClient::save_partial_download(
    const LocalPartialDownload& download,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(
        mutex_
    );
```
- 加锁。

```cpp
    static constexpr const char* sql =
        "INSERT INTO partial_downloads("
        "account_username,server_transfer_id,scope,"
        "sender_username,group_name,file_name,temp_path,"
        "file_size,sha256_hex"
        ") VALUES(?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(account_username,server_transfer_id) "
        "DO UPDATE SET "
        "scope=excluded.scope,"
        "sender_username=excluded.sender_username,"
        "group_name=excluded.group_name,"
        "file_name=excluded.file_name,"
        "temp_path=excluded.temp_path,"
        "file_size=excluded.file_size,"
        "sha256_hex=excluded.sha256_hex";
```
- SQL 语句，9 个字段。主键冲突时更新所有非主键字段（覆盖旧任务）。

```cpp
    sqlite3_stmt* raw_statement =
        nullptr;

    if (sqlite3_prepare_v2(
            database_,
            sql,
            -1,
            &raw_statement,
            nullptr
        ) != SQLITE_OK) {
        error =
            sqlite_error(database_);
        return false;
    }

    StatementPtr statement(
        raw_statement,
        sqlite3_finalize
    );
```
- 准备语句。

```cpp
    const bool bound =
        bind_text(
            statement.get(),
            1,
            download.account_username
        ) &&
        sqlite3_bind_int64(
            statement.get(),
            2,
            static_cast<sqlite3_int64>(
                download.server_transfer_id
            )
        ) == SQLITE_OK &&
        bind_text(
            statement.get(),
            3,
            download.scope
        ) &&
        bind_text(
            statement.get(),
            4,
            download.sender_username
        ) &&
        bind_text(
            statement.get(),
            5,
            download.group_name
        ) &&
        bind_text(
            statement.get(),
            6,
            download.file_name
        ) &&
        bind_text(
            statement.get(),
            7,
            download.temp_path
        ) &&
        sqlite3_bind_int64(
            statement.get(),
            8,
            static_cast<sqlite3_int64>(
                download.file_size
            )
        ) == SQLITE_OK &&
        bind_text(
            statement.get(),
            9,
            download.sha256_hex
        );
```
- 绑定 9 个参数：第 2 个是 `server_transfer_id`（`uint64_t`），第 8 个是 `file_size`，其余为字符串。

```cpp
    if (!bound) {
        error =
            sqlite_error(database_);
        return false;
    }

    if (sqlite3_step(
            statement.get()
        ) != SQLITE_DONE) {
        error =
            sqlite_error(database_);
        return false;
    }

    return true;
}
```
- 检查绑定，执行，返回结果。

---

#### 5. `get_partial_download`

**功能**：根据账户和传输 ID 查询一个下载任务，结果存入 `std::optional`。

```cpp
bool SqliteClient::get_partial_download(
    const std::string& account_username,
    std::uint64_t transfer_id,
    std::optional<LocalPartialDownload>& download,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(
        mutex_
    );
```
- 加锁。

```cpp
    static constexpr const char* sql =
        "SELECT scope,sender_username,group_name,file_name,"
        "temp_path,file_size,sha256_hex "
        "FROM partial_downloads "
        "WHERE account_username=? "
        "AND server_transfer_id=? "
        "LIMIT 1";
```
- 查询除主键外的所有字段，条件是两个主键列，`LIMIT 1` 确保最多一条结果。

```cpp
    sqlite3_stmt* raw_statement =
        nullptr;

    if (sqlite3_prepare_v2(
            database_,
            sql,
            -1,
            &raw_statement,
            nullptr
        ) != SQLITE_OK) {
        error =
            sqlite_error(database_);
        return false;
    }

    StatementPtr statement(
        raw_statement,
        sqlite3_finalize
    );
```
- 准备。

```cpp
    if (!bind_text(
            statement.get(),
            1,
            account_username
        ) ||
        sqlite3_bind_int64(
            statement.get(),
            2,
            static_cast<sqlite3_int64>(
                transfer_id
            )
        ) != SQLITE_OK) {
        error =
            sqlite_error(database_);
        return false;
    }
```
- 绑定两个参数。

```cpp
    const int result =
        sqlite3_step(
            statement.get()
        );

    if (result == SQLITE_DONE) {
        download.reset();
        return true;
    }
```
- 第一次 `step`，如果没找到（`DONE`），则重置 `optional` 为空，返回成功。

```cpp
    if (result != SQLITE_ROW) {
        error =
            sqlite_error(database_);
        return false;
    }
```
- 如果既不是 `DONE` 也不是 `ROW`，则为错误。

```cpp
    auto column_text =
        [&](int column) {
            const unsigned char* value =
                sqlite3_column_text(
                    statement.get(),
                    column
                );

            return value == nullptr
                ? std::string()
                : std::string(
                      reinterpret_cast<
                          const char*
                      >(value)
                  );
        };
```
- 同前，安全取字符串 lambda。

```cpp
    LocalPartialDownload item;
    item.server_transfer_id =
        transfer_id;
    item.account_username =
        account_username;
    item.scope =
        column_text(0);
    item.sender_username =
        column_text(1);
    item.group_name =
        column_text(2);
    item.file_name =
        column_text(3);
    item.temp_path =
        column_text(4);
    item.file_size =
        static_cast<std::uint64_t>(
            sqlite3_column_int64(
                statement.get(),
                5
            )
        );
    item.sha256_hex =
        column_text(6);

    download =
        std::move(item);
    return true;
}
```
- 从列索引 0~6 取出各字段，构造 `LocalPartialDownload` 对象，并将其移动赋值给 `optional`，最后返回成功。

---

#### 6. `remove_partial_download`

**功能**：根据账户和传输 ID 删除一个下载任务。

```cpp
bool SqliteClient::remove_partial_download(
    const std::string& account_username,
    std::uint64_t transfer_id,
    std::string& error
) {
    std::lock_guard<std::mutex> lock(
        mutex_
    );
```
- 加锁。

```cpp
    static constexpr const char* sql =
        "DELETE FROM partial_downloads "
        "WHERE account_username=? "
        "AND server_transfer_id=?";
```
- DELETE 语句。

```cpp
    sqlite3_stmt* raw_statement =
        nullptr;

    if (sqlite3_prepare_v2(
            database_,
            sql,
            -1,
            &raw_statement,
            nullptr
        ) != SQLITE_OK) {
        error =
            sqlite_error(database_);
        return false;
    }

    StatementPtr statement(
        raw_statement,
        sqlite3_finalize
    );
```
- 准备。

```cpp
    if (!bind_text(
            statement.get(),
            1,
            account_username
        ) ||
        sqlite3_bind_int64(
            statement.get(),
            2,
            static_cast<sqlite3_int64>(
                transfer_id
            )
        ) != SQLITE_OK) {
        error =
            sqlite_error(database_);
        return false;
    }
```
- 绑定两个参数。

```cpp
    if (sqlite3_step(
            statement.get()
        ) != SQLITE_DONE) {
        error =
            sqlite_error(database_);
        return false;
    }

    return true;
}
```
- 执行并返回。

---

#### ✅ 小结

新增的部分让 `SqliteClient` 具备了**管理上传队列**和**断点下载**的能力，具体来说：

- **`pending_uploads` 表**存储了尚未完成的上传请求，应用可以在网络恢复后重新尝试上传，并提供进度持久化。
- **`partial_downloads` 表**记录了已经下载了一部分文件的临时路径，便于后续继续下载，实现断点续传。

这些新增操作都遵循了相同的线程安全、错误处理和 RAII 风格，与原有代码保持高度一致。



#### 总结

这段代码实现了线程安全的 SQLite 本地缓存层，提供了：
- **打开/关闭** 数据库并自动创建目录和表结构。
- **插入/更新** 消息和文件记录（使用 `INSERT ... ON CONFLICT DO UPDATE` 实现 upsert）。
- **查询** 最近记录（按 `server_message_id` 倒序并反转升序）。
- **统计** 各类记录数量。
- 所有操作通过 `std::mutex` 保护，避免多线程竞态。
- 使用 RAII 包装 `sqlite3_stmt*`，确保语句资源自动释放，避免泄露。

每一步都严格检查 SQLite API 的返回值，并将错误信息通过 `error` 参数返回给调用者。

### minimuduo

#### NonCopyable

```c++
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
```

-   **`private`（私有的）**：只有**类自己**的成员函数能访问。外界和子类都不能访问。
-   **`protected`（受保护的）**：**类自己**和**它的子类（派生类）** 能访问。但**外界（普通代码）** 不能访问。
-   **`public`（公有的）**：任何人都能访问。

>   **规则**：只要程序员自己写了**任何**构造函数（无论是拷贝构造、移动构造，还是带参构造），编译器就**不再**自动生成“默认构造函数

我们下面自己定义了删除拷贝构造函数和拷贝赋值运算符函数，那么系统就不会自己生成默认构造函数，我们就不能新创建对象了

```
NonCopyable b;//禁止了hhh
```

所以说，我们才需要主动写构造函数

#### SocketOptions

```c++
bool configureTcpKeepAlive(
    int socketFd,//哪一个连接？
    int idleSeconds,//空闲等待时间：如果过l这个时间没有任何数据收发，操作系统开始发探测包检查对方是不是活着
    int intervalSeconds,//探测重试间隔
    int probeCount,//探测失败上限
    std::string& error
);
```

##### 为什么需要？

你（服务器）和客户端建立了一个 TCP 连接，双方正在愉快地聊天。

突然，**客户端电脑蓝屏死机了**，或者 **网线被拔掉了**。

这时候，你作为服务器，**完全感知不到这件事**。因为 TCP 连接没有发送任何“再见”的包，你这边只会觉得“对方怎么不说话了呢？”，然后傻傻地等着。

**这个“傻等”的后果很严重**：

-   你的服务器内存里会一直保留着这个连接对象（文件描述符、缓冲区）。
-   如果有几万个这样的“死连接”挂着，你的服务器内存就会爆掉，无法再服务新用户。

##### 怎么做？

“操作系统大哥，你帮我盯着这个连接。如果对方长时间不吭声（`idleSeconds`），你就主动发个探测包去戳一下它。如果戳了好几次（`probeCount`）它都没反应，你就赶紧告诉我这个程序，我好把这个死连接关掉。”

它具体干了哪 4 件事？（把代码翻译成中文）

1.  **开总闸**：`SO_KEEPALIVE` -> 把“帮我探测死连接”这个功能打开。
2.  **设定多久开始查**：`TCP_KEEPIDLE` -> 空闲多少秒后开始探测（比如 60 秒没收到数据，就去戳一下）。
3.  **设定多久查一次**：`TCP_KEEPINTVL` -> 戳了没反应，隔几秒再戳一下。
4.  **设定查几次判死刑**：`TCP_KEEPCNT` -> 连续戳几次都没回应，就判定这个连接已经死了，操作系统会把错误告诉你的程序。

特别重要的纠正：它不等于“心跳包”

你可能会问：“那我程序里自己每隔几秒发个 Ping 不也行吗？”

**这就是最关键的区别**：

-   **程序发 Ping（应用层心跳）**：如果你的程序**卡死**了（比如死锁、CPU 100%），Ping 发不出去，服务器会发现。这检测的是“你的程序有没有挂”。
-   **这个函数做的（内核保活）**：即使你的程序卡死了，**操作系统内核依然活得好好的**，它依然能发出探测包。如果客户端整个机器断电了，操作系统依然能检测到。

**所以，这个函数查的是“物理线路通不通”和“对方操作系统有没有宕机”，而不是“对方程序有没有卡住”**





```c++
bool configureTcpKeepAlive(
    int socketFd,
    int idleSeconds,
    int intervalSeconds,
    int probeCount,
    std::string& error
) {//开启保活检测，value是1
    if (!setIntOption(
            socketFd,
            SOL_SOCKET,//socket层
            SO_KEEPALIVE,
            1,
            "SO_KEEPALIVE",
            error
        )) {
        return false;
    }

#ifdef TCP_KEEPIDLE
    if (!setIntOption(
            socketFd,
            IPPROTO_TCP,//tcp层
            TCP_KEEPIDLE,//宏
            idleSeconds,
            "TCP_KEEPIDLE",
            error
        )) {
        return false;
    }
#endif

#ifdef TCP_KEEPINTVL
    if (!setIntOption(
            socketFd,
            IPPROTO_TCP,
            TCP_KEEPINTVL,
            intervalSeconds,
            "TCP_KEEPINTVL",
            error
        )) {
        return false;
    }
#endif

#ifdef TCP_KEEPCNT
    if (!setIntOption(
            socketFd,
            IPPROTO_TCP,
            TCP_KEEPCNT,
            probeCount,
            "TCP_KEEPCNT",
            error
        )) {
        return false;
    }
#endif

    return true;
}
```



| 宏名称          | 含义                       | 所属层级  |
| :-------------- | :------------------------- | :-------- |
| `SOL_SOCKET`    | “通用 Socket 层”的标识符   | Socket 层 |
| `SO_KEEPALIVE`  | “开启保活功能”这个选项     | Socket 层 |
| `IPPROTO_TCP`   | “TCP 协议层”的标识符       | TCP 层    |
| `TCP_KEEPIDLE`  | “空闲多久开始探测”这个选项 | TCP 层    |
| `TCP_KEEPINTVL` | “探测重试间隔”这个选项     | TCP 层    |
| `TCP_KEEPCNT`   | “最大探测次数”这个选项     | TCP 层    |

#### Buffer



---

##### 一、获取缓冲区状态（三个 getter）

`std::size_t readableBytes() const noexcept`

```cpp
return writerIndex_ - readerIndex_;
```
- **作用**：返回当前可读字节数。
- **实现**：直接用 `writerIndex_` 减去 `readerIndex_`，简单高效。

`std::size_t writableBytes() const noexcept`

```cpp
return buffer_.size() - writerIndex_;
```
- **作用**：返回当前可写空间大小（尾部空闲字节数）。
- **实现**：用总容量 `buffer_.size()` 减去 `writerIndex_`。

`std::size_t prependableBytes() const noexcept`

```cpp
return readerIndex_;
```
- **作用**：返回前置空间大小（即头部已消费或预留的空闲字节数）。
- **实现**：直接返回 `readerIndex_`，因为从下标 0 到 `readerIndex_ - 1` 都是空闲的（包括最初预留的 8 个字节和已消费的数据）。

---

##### 二、获取内部指针（零拷贝读取/写入）

`const char* peek() const noexcept`

```cpp
return begin() + readerIndex_;
```
- **作用**：返回可读数据的起始指针（只读）。
- **实现**：底层数组起始地址 + `readerIndex_`。

`char* beginWrite() noexcept`

```cpp
return begin() + writerIndex_;
```
- **作用**：返回可写区域的起始指针（可写），允许外部直接向该位置写入数据。

`const char* beginWrite() const noexcept`

- const 版本，返回只读指针。

---

##### 三、消费数据（retrieve 系列）

`void retrieve(std::size_t length)`

```cpp
assert(length <= readableBytes());
if (length < readableBytes()) {
    readerIndex_ += length;
} else {
    retrieveAll();
}
```
- assert:**运行时断言（Runtime Assertion）**，它的作用是**在程序运行时强行检查一个条件，如果条件不成立（即 `length` 大于当前可读数据量），程序会立即崩溃终止，并输出错误信息**

- **. 只在 Debug 模式下生效**

    `assert` 是一个**宏**，它由预处理器控制。如果你在编译时**没有**定义 `NDEBUG` 宏（通常对应 Debug 编译配置），`assert` 会生效。
    如果你**定义**了 `NDEBUG`（通常对应 Release 编译配置），**这一整行代码会被预处理器直接移除**，不会生成任何机器码，对性能零影响。

    **失败时会主动终止程序**

    一旦条件为假，`assert` 会调用 `abort()` 函数，并输出类似这样的信息：

    ```
    Assertion failed: length <= readableBytes(), file Buffer.cpp, line 123
    ```

    这让程序员能迅速定位到出错的位置。

- **作用**：丢弃 `length` 字节的数据。

- **实现**：
  
  - 如果 `length` 小于可读字节数，则简单地将 `readerIndex_` 向前移动 `length`。
  - 如果 `length` 等于或大于可读字节数，则直接调用 `retrieveAll()` 重置缓冲区（避免 `readerIndex_` 超出 `writerIndex_`）。

`void retrieveUntil(const char* end)`

```cpp
assert(peek() <= end);
assert(end <= beginWrite());
retrieve(static_cast<std::size_t>(end - peek()));
```
- **作用**：丢弃从 `peek()` 到 `end` 之间的所有数据。
- **实现**：计算需要丢弃的长度 `end - peek()`，然后调用 `retrieve(length)`。

`void retrieveAll() noexcept`

```cpp
readerIndex_ = kCheapPrepend;
writerIndex_ = kCheapPrepend;
```
- **作用**：清空缓冲区（丢弃所有可读数据）。
- **实现**：将两个指针都重置为 `kCheapPrepend`，即 8。这样，头部预留空间依然保留，所有空间都变成可写空间。

`std::string retrieveAsString(std::size_t length)`

```cpp
length = std::min(length, readableBytes());
std::string result(peek(), length);
retrieve(length);
return result;
```
- **作用**：取出 `length` 字节数据，以 `std::string` 返回，并自动丢弃这些数据。
- **实现**：先限制取出的长度不超过可读字节数，然后用 `peek()` 和 `length` 构造一个 `string`（拷贝），再调用 `retrieve(length)` 丢弃数据。

`std::string retrieveAllAsString()`

```cpp
return retrieveAsString(readableBytes());
```
- **作用**：取出所有可读数据，返回 `std::string`，并清空缓冲区。

---

##### 四、查找辅助

`const char* findEOL() const noexcept`

```cpp
const void* result = std::memchr(peek(), '\n', readableBytes());
return static_cast<const char*>(result);
```
- **作用**：在可读数据中查找第一个 `\n`（换行符）的位置。
- **实现**：调用 C 标准库 `memchr` 函数，从 `peek()` 开始搜索 `readableBytes()` 个字节。注意这里只找 `\n`，不找 `\r\n`，所以需要上层自行处理回车符（或者改进实现）。这是比较简单的实现，通常用于行协议。

---

##### 五、写入数据相关

`void ensureWritableBytes(std::size_t length)`

```cpp
if (writableBytes() < length) {
    makeSpace(length);
}
assert(writableBytes() >= length);
```
- **作用**：确保至少有 `length` 字节的可写空间，若不足则调用 `makeSpace` 进行扩容或搬移。
- **实现**：先检查当前可写字节数，不足则调用 `makeSpace`，最后断言保证空间足够。

`void hasWritten(std::size_t length)`

```cpp
assert(length <= writableBytes());
writerIndex_ += length;
```
- **作用**：在外部向 `beginWrite()` 写入数据后，调用此函数将 `writerIndex_` 向前推进 `length`，确认数据已写入。
- **实现**：先断言 `length` 不超过可写空间，然后直接累加 `writerIndex_`。

`void append(const char* data, std::size_t length)`

```cpp
ensureWritableBytes(length);
std::copy(data, data + length, beginWrite());
hasWritten(length);
```
- **作用**：将 `data` 指向的 `length` 字节数据追加到缓冲区末尾。
- **实现**：
  1. 确保可写空间足够。
  2. 用 `std::copy` 将数据拷贝到可写区域。
  3. 调用 `hasWritten` 更新 `writerIndex_`。

`void append(const std::string& data)`

```cpp
append(data.data(), data.size());
```
- **作用**：重载版本，追加 `std::string` 数据。

---

##### 六、与 Socket 的直接 I/O（关键函数）

`ssize_t readFd(int fd, int* savedErrno)`

这是最核心、最精妙的部分，它利用 **`readv` 系统调用（分散读）** 来最大化一次读取的数据量，减少系统调用次数。

```cpp
char extraBuffer[65536];               // 栈上临时缓冲区，64KB

iovec vectors[2];                      // readv 需要两个缓冲区
const std::size_t writable = writableBytes();

vectors[0].iov_base = beginWrite();    // 第一块：Buffer 自身的可写区域
vectors[0].iov_len = writable;
vectors[1].iov_base = extraBuffer;     // 第二块：栈上额外缓冲区
vectors[1].iov_len = sizeof(extraBuffer);

const int vectorCount = writable < sizeof(extraBuffer) ? 2 : 1;
const ssize_t bytesRead = ::readv(fd, vectors, vectorCount);
```
- **核心逻辑**：如果 `Buffer` 自身的可写空间小于 64KB，则一次调用 `readv` 同时读入两个位置：先填满 Buffer 自身，剩下的读入栈上的 `extraBuffer`。这样一次系统调用可以读取多达 `writable + 64KB` 的数据，极大提高效率。
- **为什么用栈缓冲区？** 因为如果 Buffer 的可写空间很小（比如只剩几百字节），我们仍想一次读取大量数据，避免多次 `read` 调用。栈上的 `extraBuffer` 提供了额外的暂存空间。

**处理读取结果：**
```cpp
if (bytesRead < 0) {
    *savedErrno = errno;
} else if (static_cast<std::size_t>(bytesRead) <= writable) {
    writerIndex_ += static_cast<std::size_t>(bytesRead);   // 全部落入 Buffer 自身
} else {
    writerIndex_ = buffer_.size();                         // Buffer 自身已满
    append(extraBuffer, static_cast<std::size_t>(bytesRead) - writable); // 剩余数据追加到 Buffer
}
return bytesRead;
```
- 如果读到的数据小于或等于 Buffer 的可写空间，全部直接写入 Buffer，更新 `writerIndex_`。
- 如果读到的数据超过 Buffer 的可写空间，先将 Buffer 自身填满（`writerIndex_` 移到末尾），然后把 `extraBuffer` 中的剩余部分通过 `append` 追加到 Buffer（`append` 内部会调用 `ensureWritableBytes`，可能触发扩容或搬移）。
- **这样设计的好处**：一次 `readv` 可以读完 socket 接收缓冲区中的所有数据，减少 `epoll` 循环次数。

`ssize_t writeFd(int fd, int* savedErrno)`

```cpp
const ssize_t bytesWritten = ::write(fd, peek(), readableBytes());
if (bytesWritten < 0) {
    *savedErrno = errno;
}
return bytesWritten;
```
- **作用**：尝试将 Buffer 中的所有可读数据写入 socket。
- **实现**：调用 `write` 系统调用，写入 `peek()` 起始的 `readableBytes()` 字节。如果写入成功，返回实际写入字节数；外部需要根据返回值调用 `retrieve` 来更新 `readerIndex_`。如果返回值小于 `readableBytes()`，说明 socket 发送缓冲区已满，需要等待 `EPOLLOUT` 事件。

---

##### 七、私有辅助函数

`char* begin() noexcept`

```cpp
return buffer_.data();
```
- **作用**：返回底层 `vector<char>` 的起始地址。

`const char* begin() const noexcept`

const 版本。

`void makeSpace(std::size_t length)`

这是缓冲区的空间管理核心，决定是**搬移数据**还是**扩容**。

```cpp
if (writableBytes() + prependableBytes() < length + kCheapPrepend) {
    buffer_.resize(writerIndex_ + length);
    return;
}
```
- **判断条件**：如果 **“尾部空间 + 前置空间”** 小于 **“需要空间 + 头部预留”**，说明即使把可读数据搬到最前面，也无法腾出足够的连续空间，因此必须扩容。扩容后大小至少为 `writerIndex_ + length`（保留当前已写入数据）。

否则，可以进行搬移：
```cpp
assert(kCheapPrepend < readerIndex_);
const std::size_t readable = readableBytes();
std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
readerIndex_ = kCheapPrepend;
writerIndex_ = readerIndex_ + readable;
```
- **搬移操作**：将当前可读数据从原来的位置（`readerIndex_`）**拷贝到缓冲区的起始偏移 `kCheapPrepend`（即 8）处**，覆盖掉之前的前置空间和已消费数据。
- **更新指针**：`readerIndex_` 重置为 8，`writerIndex_` 设置为 `8 + readable`。
- **效果**：释放了尾部大量空间，因为所有可读数据现在都紧挨着头部预留区，尾部变得连续可用。
- **注意**：这里只用 `std::copy` 拷贝可读数据，并不移动已消费的数据，所以效率很高（只是内存拷贝，大小等于当前未读数据量）。

---

##### 八、设计精要总结

1. **`readFd` 的 `readv` 技巧**：利用栈上额外缓冲区，一次系统调用即可读完大量数据，显著提升吞吐量。
2. **`makeSpace` 的搬移策略**：优先通过搬移数据释放尾部空间，避免频繁扩容带来的内存重分配和拷贝。
3. **前置预留 `kCheapPrepend`**：始终保留 8 字节头部，使得“在数据前追加协议头”变得零成本。
4. **`retrieve` 不真正清除内存**：仅仅移动指针，高效且可重复利用已分配的内存。
5. **`ensureWritableBytes` + `hasWritten` 分离**：允许外部直接写入 `beginWrite()` 区域，避免额外拷贝。

---

##### 九、使用示例回顾

- **接收数据**：调用 `readFd`，数据进入 Buffer。
- **解析**：用 `peek()` 和 `readableBytes()` 查看数据，用 `findEOL()` 查找行尾。
- **消费**：调用 `retrieveAsString(length)` 取出完整包并丢弃。
- **发送**：调用 `append(data)` 将响应写入 Buffer，然后在可写事件中调用 `writeFd` 发送。

至此，整个 Buffer 类的实现已剖析完毕。如果你对某个细节（比如 `std::copy` 的异常安全，或 `readv` 的性能对比）还有疑问，可以随时追问。😊





#### POller



```c++
namespace {
[[noreturn]] void throwSystemError(const char* operation) {
    throw std::runtime_error(
        std::string(operation) + " failed: " + std::strerror(errno));
}
}  // namespace
```



-   **`throwSystemError`**：一个内部辅助函数，用于在系统调用失败时抛出异常。
-   **参数 `operation`**：描述发生错误时正在执行的操作名称（如 `"epoll_create1"`、`"epoll_wait"`、`"epoll_ctl"`）。
-   **行为**：拼接错误信息（操作名 + `strerror(errno)`），然后抛出一个 `std::runtime_error` 异常。
-   **`[[noreturn]]`**：C++11 属性，表示该函数不会返回（因为它总是抛出异常），帮助编译器优化。
-   **设计意图**：简化错误处理，避免在多个地方重复编写 `if (xxx < 0) throw ...` 样板代码。



#### EventLoop



##### 一、匿名命名空间中的辅助函数

```cpp
namespace {
int createEventFd() {
    const int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd < 0) {
        throw std::runtime_error(
            std::string("eventfd failed: ") + std::strerror(errno));
    }
    return fd;
}
}  // namespace
```

- **作用**：创建一个 `eventfd` 文件描述符，用于 **跨线程唤醒** 事件循环。
- **`eventfd`** 是 Linux 内核提供的一种轻量级事件通知机制，本质上是一个计数器，可以通过 `read`/`write` 原子地修改。
- **参数**：
  - `0`：初始计数值。
  - `EFD_NONBLOCK`：设置非阻塞模式。
  - `EFD_CLOEXEC`：当执行 `exec` 时自动关闭该描述符，防止子进程继承。
- **错误处理**：如果创建失败（`fd < 0`），抛出 `std::runtime_error` 异常。

##### 二、构造函数 `EventLoop::EventLoop()`

```cpp
EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(std::this_thread::get_id()),
      poller_(std::make_unique<Poller>(this)),
      wakeupFd_(createEventFd()),
      wakeupChannel_(std::make_unique<Channel>(this, wakeupFd_)) {
    wakeupChannel_->setReadCallback([this] { handleWakeupRead(); });
    wakeupChannel_->enableReading();
}
```

- **作用**：创建一个 `EventLoop` 对象，初始化所有状态，并设置唤醒机制。
- **初始化列表**：
  - `looping_(false)`：表示当前未进入事件循环。
  - `quit_(false)`：退出标志。
  - `callingPendingFunctors_(false)`：表示当前未执行待处理任务队列。
  - `threadId_(std::this_thread::get_id())`：**记录当前线程 ID**，用于后续所有线程安全性检查。
  - `poller_(std::make_unique<Poller>(this))`：创建 `Poller` 对象（`epoll` 封装），并传入 `this` 指针，方便 `Poller` 进行线程断言。
  - `wakeupFd_(createEventFd())`：调用辅助函数创建 `eventfd` 描述符。
  - `wakeupChannel_(std::make_unique<Channel>(this, wakeupFd_))`：为 `wakeupFd_` 创建一个 `Channel`，并注册到本 `EventLoop`。
- **构造函数体**：
  - 设置 `wakeupChannel_` 的读回调为 `handleWakeupRead`（当 `eventfd` 可读时触发）。
  - 调用 `wakeupChannel_->enableReading()`，让 `Poller` 监听 `wakeupFd_` 的读事件。这样，当其他线程向 `eventfd` 写入数据时，该 Channel 就会被激活，从而唤醒 `epoll_wait`。

---

##### 三、析构函数 `EventLoop::~EventLoop()`

```cpp
EventLoop::~EventLoop() {
    assertInLoopThread();
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
}
```

- **作用**：析构 `EventLoop` 对象，清理资源。
- **流程**：
  1. 断言当前线程是所属线程，禁止跨线程析构。
  2. 关闭 `wakeupChannel_` 的所有事件（`disableAll`）。
  3. 将 `wakeupChannel_` 从 `Poller` 中移除（`remove`）。
  4. 关闭 `wakeupFd_` 文件描述符。

---

##### 四、核心事件循环 `void EventLoop::loop()`

这是 **最核心的函数**，负责驱动整个网络库的运行。

```cpp
void EventLoop::loop() {
    assertInLoopThread();

    bool expected = false;
    if (!looping_.compare_exchange_strong(expected, true)) {
        throw std::logic_error("EventLoop::loop called while already looping");
    }

    quit_.store(false);
    while (!quit_.load()) {
        activeChannels_.clear();
        poller_->poll(std::chrono::milliseconds(10000), &activeChannels_);

        for (Channel* channel : activeChannels_) {
            channel->handleEvent();
        }

        doPendingFunctors();
    }

    looping_.store(false);
}
```

- **作用**：进入主事件循环，持续监听 I/O 事件并执行回调，直到调用 `quit()`。
- **参数**：无。
- **内部逻辑**：
  1. **线程断言**：确保在所属线程中调用。
  2. **防重入**：使用 `compare_exchange_strong` 原子操作，将 `looping_` 从 `false` 设为 `true`。如果已经为 `true`（说明循环已在运行），则抛出 `logic_error`。
  3. **重置退出标志**：将 `quit_` 设为 `false`。
  4. **进入主循环**：
     - 清空 `activeChannels_`（`vector<Channel*>`），准备存放本次就绪的 Channel。
     - 调用 `poller_->poll(timeout, &activeChannels_)`，**阻塞等待 I/O 事件**。超时时间为 **10 秒（10000 毫秒）**。
       - 如果 10 秒内没有事件发生，`poll` 返回空列表，循环继续。
       - 如果有事件，`activeChannels_` 被填充为所有就绪的 Channel。
     - 遍历 `activeChannels_`，依次调用每个 Channel 的 `handleEvent()`，由 Channel 内部根据 `revents_` 执行对应的读/写/关闭回调。
     - 调用 `doPendingFunctors()`，执行从其他线程投递的任务（如 `runInLoop` 投递的函数）。
  5. **退出循环**：当 `quit_` 被设为 `true` 时，`while` 条件失败，退出循环。
  6. 将 `looping_` 设为 `false`，表示循环已结束。

**设计要点**：
- **超时时间 10 秒**：确保即使没有 I/O 事件，循环也能定期执行 `doPendingFunctors()`，从而及时处理跨线程任务。
- **事件处理顺序**：先处理 I/O 事件，再处理任务队列，这样不会因为任务队列中的耗时操作延迟 I/O 响应。
- **使用原子变量**：`looping_` 和 `quit_` 使用 `std::atomic`，确保跨线程可见性。

---

##### 五、退出循环 `void EventLoop::quit()`

```cpp
void EventLoop::quit() {
    quit_.store(true);
    if (!isInLoopThread()) {
        wakeup();
    }
}
```

- **作用**：请求退出事件循环。
- **参数**：无。
- **内部逻辑**：
  1. 将 `quit_` 原子地设为 `true`。
  2. 如果当前调用线程 **不是** 所属线程（即从其他线程调用 `quit`），则调用 `wakeup()` 唤醒事件循环。因为事件循环可能在 `poller_->poll()` 中阻塞，不唤醒它，循环无法及时检查 `quit_` 状态而退出。
- **设计要点**：如果 `quit()` 从所属线程调用，那么当前循环没有阻塞（因为循环在 `poll` 中阻塞时，所属线程无法执行其他任务），所以不需要唤醒，下一次循环迭代会自然退出。

---

##### 六、跨线程任务投递

1. `void EventLoop::runInLoop(Functor callback)`

```cpp
void EventLoop::runInLoop(Functor callback) {
    if (isInLoopThread()) {
        callback();
    } else {
        queueInLoop(std::move(callback));
    }
}
```

- **作用**：在事件循环线程中执行回调函数。如果当前已经在循环线程中，则立即执行；否则将回调加入队列，等待循环线程执行。
- **参数 `callback`**：`std::function<void()>` 类型的可调用对象。
- **内部逻辑**：
  - 如果在所属线程，直接调用 `callback()`。
  - 否则，调用 `queueInLoop(std::move(callback))` 将回调加入队列，并唤醒循环线程。

2. `void EventLoop::queueInLoop(Functor callback)`

```cpp
void EventLoop::queueInLoop(Functor callback) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(callback));
    }

    if (!isInLoopThread() || callingPendingFunctors_.load()) {
        wakeup();
    }
}
```

- **作用**：将回调加入 `pendingFunctors_` 队列，并在必要时唤醒事件循环。
- **参数 `callback`**：要加入队列的回调。
- **内部逻辑**：
  1. 加锁，将 `callback` 移动（`std::move`）到 `pendingFunctors_` 向量末尾。
  2. 判断是否需要唤醒：
     - **如果当前线程不是所属线程**，必须唤醒（因为循环可能在阻塞）。
     - **如果 `callingPendingFunctors_` 为 `true`**，说明当前循环线程正在执行上一轮任务队列中的回调，而此时新任务被加入队列，需要唤醒以确保新任务在**本轮循环结束后的下一次循环**中得到执行（因为当前循环末尾的 `doPendingFunctors()` 已经执行过了，新任务要等到下一轮循环的末尾才被执行，如果不唤醒，可能需要等待 10 秒的超时）。
- **设计要点**：通过 `callingPendingFunctors_` 标志，避免了不必要的唤醒：如果循环线程正在执行任务，且新任务是在循环线程内部加入的（由当前正在执行的任务生成），此时循环并没有阻塞，无需唤醒，因为下一轮循环会处理队列。

3. `void EventLoop::doPendingFunctors()`

```cpp
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_.store(true);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor& functor : functors) {
        functor();
    }

    callingPendingFunctors_.store(false);
}
```

- **作用**：执行所有待处理的回调函数。
- **参数**：无。
- **内部逻辑**：
  1. 设置 `callingPendingFunctors_` 为 `true`，表示正在执行任务队列。
  2. 加锁，将 `pendingFunctors_` 与局部向量 `functors` **交换（`swap`）**。
     - 此时 `pendingFunctors_` 变为空，后续其他线程或本线程加入的新任务会进入空的 `pendingFunctors_`。
     - `functors` 持有所有旧任务。
  3. 释放锁，然后遍历 `functors` 并依次执行每个回调。
  4. 执行完毕后，将 `callingPendingFunctors_` 设为 `false`。
- **设计要点**：
  - **减少锁的持有时间**：只在交换数据时加锁，执行回调时不需要持有锁，因为回调可能耗时，这会避免阻塞其他线程投递任务。
  - **`swap` 技巧**：先取出所有任务，再执行，防止在任务执行过程中，新任务被追加到同一个向量导致迭代器失效。

---

##### 七、Channel 管理（代理给 Poller）

`void EventLoop::updateChannel(Channel* channel)`

```cpp
void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    poller_->updateChannel(channel);
}
```

- **作用**：更新一个 Channel 在 `Poller` 中的事件监听状态（添加/修改/删除）。由 `Channel::update()` 调用。
- **参数 `channel`**：要更新的 Channel 指针。
- **逻辑**：断言该 Channel 确实属于本 EventLoop（防止误操作），然后委托给 `Poller::updateChannel`。

`void EventLoop::removeChannel(Channel* channel)`

```cpp
void EventLoop::removeChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    poller_->removeChannel(channel);
}
```

- **作用**：从 `Poller` 中永久移除一个 Channel。由 `Channel::remove()` 调用。
- **参数**：要移除的 Channel 指针。
- **逻辑**：断言归属后，委托给 `Poller::removeChannel`。

---

##### 八、线程身份验证

`bool EventLoop::isInLoopThread() const noexcept`

```cpp
return threadId_ == std::this_thread::get_id();
```

- **作用**：判断当前线程是否是该 EventLoop 的所属线程。
- **返回值**：`true` 表示当前线程是所属线程，`false` 表示不是。

`void EventLoop::assertInLoopThread() const`

```cpp
if (!isInLoopThread()) {
    throw std::logic_error("EventLoop used from a foreign thread");
}
```

- **作用**：断言当前线程是所属线程，否则抛出 `std::logic_error`。
- **用途**：在需要线程安全的函数入口处调用，确保所有操作都在正确的线程中执行。

---

##### 九、唤醒机制（`wakeup` 和 `handleWakeupRead`）

`void EventLoop::wakeup()`

```cpp
void EventLoop::wakeup() {
    const std::uint64_t one = 1;
    const ssize_t written = ::write(wakeupFd_, &one, sizeof(one));
    if (written < 0 && errno != EAGAIN) {
        throw std::runtime_error(
            std::string("eventfd write failed: ") + std::strerror(errno));
    }
}
```

- **作用**：向 `eventfd` 写入一个 8 字节的整数（`1`），从而唤醒可能在 `poll` 中阻塞的事件循环。
- **内部逻辑**：调用 `write` 写入 `1`。如果写入失败且错误不是 `EAGAIN`（表示非阻塞写可能暂时不可用），则抛出异常。

`void EventLoop::handleWakeupRead()`

```cpp
void EventLoop::handleWakeupRead() {
    std::uint64_t one = 0;
    const ssize_t readBytes = ::read(wakeupFd_, &one, sizeof(one));
    if (readBytes < 0 && errno != EAGAIN) {
        throw std::runtime_error(
            std::string("eventfd read failed: ") + std::strerror(errno));
    }
}
```

- **作用**：处理 `eventfd` 的可读事件，读取并丢弃计数器的值（即消耗唤醒事件）。
- **调用**：由 `wakeupChannel_` 的读回调触发（在构造函数中设置）。
- **内部逻辑**：调用 `read` 读取 8 字节到 `one`（忽略其值），目的是清空事件计数。如果读取失败且错误不是 `EAGAIN`，则抛出异常。
- **设计要点**：因为 `eventfd` 的计数器是累加的，每次写入 1，读取后会将计数器清零（如果读走了所有值）。这里一次读取 8 字节，会消费掉所有当前累积的计数，防止重复唤醒。

---

##### 十、总结：`EventLoop` 的整体功能

| 功能模块         | 关键成员/函数                                                | 说明                                 |
| :--------------- | :----------------------------------------------------------- | :----------------------------------- |
| **主事件循环**   | `loop()`                                                     | 无限循环，驱动事件处理               |
| **退出控制**     | `quit()`                                                     | 安全退出循环                         |
| **I/O 复用**     | `poller_`，`poll()`                                          | 封装 epoll，监听事件                 |
| **跨线程唤醒**   | `wakeupFd_`，`wakeupChannel_`，`wakeup()`                    | 通过 `eventfd` 唤醒 `epoll_wait`     |
| **任务队列**     | `pendingFunctors_`，`runInLoop()`，`queueInLoop()`，`doPendingFunctors()` | 支持跨线程投递任务，在循环线程执行   |
| **线程模型**     | `threadId_`，`isInLoopThread()`，`assertInLoopThread()`      | 确保线程安全，所有操作在所属线程执行 |
| **Channel 管理** | `updateChannel()`，`removeChannel()`                         | 代理给 `Poller`，管理事件注册        |

**一句话概括**：`EventLoop` 是 mini muduo 网络库的 **“大脑”** ，它通过 `Poller` 监听 I/O 事件，通过 `eventfd` 支持跨线程唤醒，通过任务队列实现异步任务执行，为上层 `TcpServer` 和 `TcpConnection` 提供了可靠的事件驱动基础设施。😊

#### EventLoopThread

---

###### 一、构造函数与析构函数

`EventLoopThread::EventLoopThread(std::string name)`

```cpp
EventLoopThread::EventLoopThread(std::string name)
    : loop_(nullptr),
      exiting_(false),
      name_(std::move(name)) {}
```
- **作用**：初始化成员变量。
- **关键点**：
  - `loop_` 初始为 `nullptr`，因为此时子线程尚未启动，`EventLoop` 对象还没被创建。
  - `name_` 通过 `std::move` 移入，避免字符串拷贝。

`EventLoopThread::~EventLoopThread()`

```cpp
EventLoopThread::~EventLoopThread() {
    exiting_ = true;

    EventLoop* loop = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop = loop_;
    }

    if (loop != nullptr) {
        loop->quit();
    }
    if (thread_.joinable()) {
        thread_.join();
    }
}
```
- **作用**：析构时安全地停止子线程并等待其退出。
- **流程**：
  1. 设置 `exiting_ = true`（虽然当前代码未使用该变量，但它为将来可能的扩展留出空间，比如在 `threadFunction` 中检查它）。
  2. **加锁**，获取 `loop_` 指针的当前值（子线程可能正在运行或已经退出）。
  3. 如果 `loop_` 非空，调用 `loop->quit()`，这会唤醒可能阻塞在 `epoll_wait` 上的事件循环，让它退出。
  4. 调用 `thread_.join()`，**阻塞等待子线程完全结束**（确保在析构函数返回前，子线程已经清理干净）。
- **设计要点**：先 `quit`，再 `join`，这是标准的优雅终止流程。

---

###### 二、启动循环：`EventLoop* EventLoopThread::startLoop()`

这是 **主线程（调用者线程）调用的关键接口**，用于启动子线程并返回 `EventLoop` 指针。

```cpp
EventLoop* EventLoopThread::startLoop() {
    thread_ = std::thread([this] { threadFunction(); });

    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] { return loop_ != nullptr; });
    return loop_;
}
```

- **步骤 1：启动子线程**
  - `std::thread([this] { threadFunction(); });`：创建一个新线程，执行成员函数 `threadFunction`。
  - **注意**：这里使用 Lambda 捕获 `this`，确保子线程能访问当前对象的成员。

- **步骤 2：等待 `loop_` 初始化完成**（这是最精妙的部分）
  - 使用 `std::unique_lock<std::mutex>` 上锁。
  - 调用 `condition_.wait(lock, predicate)`，其中 **predicate（条件）是 `[this] { return loop_ != nullptr; }`**。
  - **这意味着**：当前线程（主线程）会**释放锁并阻塞**，直到子线程在 `threadFunction` 中将 `loop_` 赋值为非空，并调用 `condition_.notify_one()` 通知它。
- **步骤 3**：当等待结束（`loop_` 已赋值），直接返回 `loop_` 指针。

**为什么必须这么做？**
因为如果不等待，主线程在 `startLoop` 返回后立即调用 `loop_->runInLoop(...)`，而此时子线程可能还没有完成 `EventLoop loop;` 的构造，`loop_` 仍为 `nullptr`，就会导致**空指针访问**。

---

###### 三、子线程入口函数：`void EventLoopThread::threadFunction()`

这是 **子线程执行的函数**，它在独立线程中创建 `EventLoop` 并进入事件循环。

```cpp
void EventLoopThread::threadFunction() {
#if defined(__linux__)
    if (!name_.empty()) {
        const std::string shortName = name_.substr(0, 15);
        (void)::pthread_setname_np(::pthread_self(), shortName.c_str());
    }
#endif

    EventLoop loop;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        condition_.notify_one();
    }

    loop.loop();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = nullptr;
    }
}
```

- **步骤 1：设置线程名称（Linux 专用）**
  - 调用 `pthread_setname_np` 给线程命名（最大 15 字符），方便 `top -H` 或 `gdb` 调试时识别。

- **步骤 2：在栈上创建 `EventLoop` 对象**
  - `EventLoop loop;` —— **注意！** 这是一个局部变量，分配在子线程的**栈**上。
  - 这也解释了为什么 `loop_` 是一个原始指针而不是 `shared_ptr`：因为 `EventLoop` 对象的生命周期与子线程绑定，线程结束时栈被销毁，指针自然失效。

- **步骤 3：通知主线程（关键同步点）**
  - 加锁，将 `loop_` 设为 `&loop`。
  - 调用 `condition_.notify_one()`，唤醒在 `startLoop` 中阻塞的主线程。
  - **解锁**（`lock_guard` 出作用域自动释放）。

- **步骤 4：进入事件循环**
  - `loop.loop();` —— 这会**阻塞**当前线程，直到 `loop.quit()` 被调用。子线程将一直在这里处理网络事件和跨线程任务。

- **步骤 5：清理**
  - 当 `loop.loop()` 返回（即 `quit` 被调用），再次加锁，将 `loop_` 设为 `nullptr`。
  - 函数执行完毕，子线程结束。

---

###### 四、一个完整的使用示例（理解其价值）

```cpp
// 主线程
EventLoopThread ioThread("IOThread");
EventLoop* loop = ioThread.startLoop();  // 阻塞直到子线程就绪

// 现在可以安全地向子线程投递任务
loop->runInLoop([] { 
    // 这段代码会在子线程中执行
    std::cout << "Hello from IO thread!" << std::endl; 
});

// 主线程继续做其他事...
// 程序退出时，ioThread 析构会自动 quit + join
```

**执行时序图：**
```text
主线程                         子线程
  |                             |
  | startLoop()                 |
  | 创建子线程  ----------------> 执行 threadFunction()
  |                             | 创建 EventLoop loop (栈上)
  | wait(condition) ---阻塞---  | loop_ = &loop; notify()
  | <------被唤醒------         | loop.loop() (开始处理事件)
  | 返回 loop_                  |
  | 调用 loop->runInLoop(...)  | (接收并执行任务)
  |                             |
  | 析构 EventLoopThread       | loop.quit() (被主线程调用)
  | loop->quit()              | loop.loop() 返回
  | thread_.join() --等待----  | 清理 loop_ = nullptr
  | <------线程退出------      | 子线程结束
```

---

###### 五、设计亮点总结

| 设计点                        | 说明                                                        |
| :---------------------------- | :---------------------------------------------------------- |
| **栈上 `EventLoop`**          | 避免动态分配（`new`），减少内存管理开销，且生命周期明确。   |
| **`condition_variable` 同步** | 确保主线程在子线程完全初始化前不会错误地使用 `loop_` 指针。 |
| **`std::move(name_)`**        | 减少字符串拷贝，体现现代 C++ 的高效风格。                   |
| **`pthread_setname_np`**      | 调试友好，便于排查多线程问题。                              |
| **析构顺序**                  | 先 `quit`，再 `join`，确保子线程安全退出，避免资源泄露。    |

**一句话总结**：`EventLoopThread` 是 muduo 多线程模型的基础砖块，它封装了“一个线程 + 一个事件循环”的配对，并通过精巧的同步机制让主线程能安全获取子线程的事件循环指针，从而将多线程网络编程的复杂性降低到“调用 `runInLoop` 投递任务”的简单程度。😊



#### EventLoopThreadPool



这段代码是 **`EventLoopThreadPool`（事件循环线程池）** 的完整实现。它的核心作用是：**管理一组 `EventLoopThread` 对象，并为上层（如 `TcpServer`）提供“从线程池中选择一个 EventLoop”的接口，以实现多线程 I/O 负载均衡。**

你可以把它理解为一个 **“I/O 线程池”**，专门负责管理和分配多个事件循环线程。

---

##### 一、构造函数与析构函数

`EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, std::string name)`

```cpp
EventLoopThreadPool::EventLoopThreadPool(
    EventLoop* baseLoop,
    std::string name)
    : baseLoop_(baseLoop),
      name_(std::move(name)),
      started_(false),
      threadCount_(0),
      next_(0) {}
```

- **作用**：初始化线程池。
- **参数**：
  - `baseLoop`：**主事件循环**（通常是 `TcpServer` 所属的 EventLoop，用于监听新连接）。
  - `name`：线程池名称前缀（用于给子线程命名，方便调试）。
- **成员初始化**：
  - `started_ = false`：尚未启动。
  - `threadCount_ = 0`：默认无子线程（此时所有 I/O 任务都在 `baseLoop` 中执行）。
  - `next_ = 0`：用于轮询（Round-Robin）分配下一个 EventLoop 的索引。

`EventLoopThreadPool::~EventLoopThreadPool() = default;`

- **作用**：默认析构函数。由于使用了 `std::unique_ptr<EventLoopThread>`，当 `threads_` 容器销毁时，每个 `EventLoopThread` 对象会被自动析构，从而安全停止子线程。

---

##### 二、设置线程数量：`void setThreadNum(int threadCount)`

```cpp
void EventLoopThreadPool::setThreadNum(int threadCount) {
    if (started_) {
        throw std::logic_error("cannot change thread count after start");
    }
    if (threadCount < 0) {
        throw std::invalid_argument("thread count cannot be negative");
    }
    threadCount_ = threadCount;
}
```

- **作用**：设置线程池中 I/O 线程的数量。
- **限制**：
  - **必须在 `start()` 之前调用**。一旦线程池启动（`started_ == true`），再修改线程数会抛出 `logic_error`，因为改变线程数意味着需要销毁并重建线程对象，涉及复杂的资源迁移。
  - `threadCount` 不能为负数。
- **默认值为 0**：表示不创建任何子线程，所有连接（包括新连接和已建立连接）都在 `baseLoop_`（主线程）中处理。这种模式下，所有 I/O 事件串行化，适合简单场景或单核环境。

---

##### 三、启动线程池：`void EventLoopThreadPool::start()`

```cpp
void EventLoopThreadPool::start() {
    baseLoop_->assertInLoopThread();
    if (started_) {
        return;
    }

    started_ = true;
    for (int index = 0; index < threadCount_; ++index) {
        auto thread = std::make_unique<EventLoopThread>(
            name_ + "-io-" + std::to_string(index));
        loops_.push_back(thread->startLoop());
        threads_.push_back(std::move(thread));
    }
}
```

- **作用**：根据 `threadCount_` 创建并启动所有 I/O 线程。
- **流程**：
  1. 断言当前线程是 `baseLoop_` 所属线程（`baseLoop_->assertInLoopThread()`），因为线程池的启动必须在主事件循环线程中进行。
  2. 如果已经启动，直接返回（幂等性保护）。
  3. 设置 `started_ = true`。
  4. **循环 `threadCount_` 次**：
     - 创建 `EventLoopThread` 对象，线程名格式如 `"MainLoop-io-0"`。
     - 调用 `thread->startLoop()`，这会**阻塞等待子线程完成初始化**，并返回该子线程中的 `EventLoop*` 指针。
     - 将返回的 `EventLoop*` 存入 `loops_` 向量（用于后续分配）。
     - 将 `EventLoopThread` 对象的所有权存入 `threads_` 向量（用于析构时清理）。
- **结果**：
  - `loops_` 中存放了所有子线程的 `EventLoop*`。
  - `threads_` 中存放了所有 `EventLoopThread` 对象（用于生命周期管理）。

---

##### 四、获取下一个 EventLoop：`EventLoop* EventLoopThreadPool::getNextLoop()`

这是 **最常用的接口**，用于在新连接建立时，从线程池中选出一个 EventLoop 来负责这个连接。

```cpp
EventLoop* EventLoopThreadPool::getNextLoop() {
    baseLoop_->assertInLoopThread();

    if (loops_.empty()) {
        return baseLoop_;
    }

    EventLoop* loop = loops_[next_];
    next_ = (next_ + 1U) % loops_.size();
    return loop;
}
```

- **作用**：以 **轮询（Round-Robin）** 的方式返回下一个 EventLoop，从而实现 I/O 负载均衡。
- **流程**：
  1. 断言当前线程是 `baseLoop_` 所属线程（确保线程安全）。
  2. 如果 `loops_` 为空（即没有创建子线程），返回 `baseLoop_`（所有连接由主线程处理）。
  3. 否则，取出 `loops_[next_]` 指向的 EventLoop，然后 `next_` 自增并取模（循环遍历）。
  4. 返回选中的 EventLoop 指针。
- **使用场景**：在 `TcpServer::newConnection` 中，调用此函数获取一个 EventLoop，然后将新连接的 `TcpConnection` 对象绑定到该 EventLoop 上。

---

##### 五、获取所有 EventLoop：`const std::vector<EventLoop*>& EventLoopThreadPool::getAllLoops() const noexcept`

```cpp
const std::vector<EventLoop*>& EventLoopThreadPool::getAllLoops() const noexcept {
    return loops_;
}
```

- **作用**：返回所有子线程 EventLoop 的列表（只读引用）。
- **用途**：在需要向所有 I/O 线程广播任务时使用（例如关闭所有连接、统计信息等）。

---

##### 六、类的整体设计模型

下面的时序图展示了 `TcpServer` 如何使用 `EventLoopThreadPool`：

```text
[主线程]                           [子线程 0]            [子线程 1]
  |                                    |                    |
  | TcpServer 构造                      |                    |
  | 创建 EventLoopThreadPool            |                    |
  | (baseLoop = 主线程 EventLoop)       |                    |
  | pool->setThreadNum(2)              |                    |
  | pool->start()                      |                    |
  |   -> 创建 EventLoopThread 0        |                    |
  |   -> thread->startLoop() --阻塞--> | 开始运行           |
  |   <----- 返回 loop_0 -------       | (EventLoop loop)   |
  |   -> 创建 EventLoopThread 1        |                    |
  |   -> thread->startLoop() --阻塞--> |                    | 开始运行
  |   <----- 返回 loop_1 -------       |                    | (EventLoop loop)
  |                                    |                    |
  | 新连接到达                         |                    |
  | pool->getNextLoop() -> 返回 loop_0 |                    |
  | 将连接绑定到 loop_0 的 Channel     |                    |
  |                                    | <-- 处理该连接的事件 |
  | 新连接到达                         |                    |
  | pool->getNextLoop() -> 返回 loop_1 |                    |
  | 将连接绑定到 loop_1 的 Channel     |                    |
  |                                    |                    | <-- 处理该连接的事件
```

---

##### 七、设计亮点

| 特性                  | 实现方式                                                     | 优势                                                         |
| :-------------------- | :----------------------------------------------------------- | :----------------------------------------------------------- |
| **主从 Reactor 模型** | `baseLoop_` 负责监听新连接，子线程负责处理已建立连接。       | 将 I/O 负载分散到多个线程，提高并发能力。                    |
| **轮询负载均衡**      | `getNextLoop()` 使用 `next_` 索引循环遍历。                  | 将新连接均匀分配到各个 I/O 线程，避免某个线程过载。          |
| **线程安全**          | `start()` 和 `getNextLoop()` 都断言在 `baseLoop_` 线程中执行。 | 保证操作不会发生数据竞争（所有对线程池的修改和查询都在主线程进行）。 |
| **灵活的单线程模式**  | `threadCount_` 默认为 0。                                    | 对于小规模应用，无需创建额外线程，降低复杂性。               |
| **RAII 资源管理**     | 使用 `std::unique_ptr<EventLoopThread>`。                    | 自动管理线程生命周期，析构时自动停止并等待线程退出。         |

---

##### 八、总结

**`EventLoopThreadPool` 是 muduo 多线程模型的核心组件，它实现了 `one loop per thread` 思想。**

- 它负责**创建和管理多个 I/O 线程**（每个线程持有一个 `EventLoop`）。
- 它通过**轮询算法**将新连接均匀分配给各个子线程。
- 它提供了**向所有线程广播任务**的能力（通过 `getAllLoops()`）。
- 它的设计让 `TcpServer` 能够轻松支持多线程，而无需关心线程创建、同步和销毁的细节。

**一句话概括**：`EventLoopThreadPool` 是 muduo 的 **“I/O 线程管家”**，你告诉它需要几个线程，它帮你启动好，然后你每次有新连接，跟它要一个 EventLoop 就行。😊



#### Acceptor

好的，这是 **`Acceptor` 类的实现**。`Acceptor` 是 mini muduo 网络库中专门负责 **“接受（Accept）新连接”** 的组件。它的核心职责是：**监听一个 TCP 端口，当有新连接到达时，接受该连接，并将新建立的 socket 文件描述符（fd）交给上层业务处理。**

可以把它理解为 **“门卫”**——它只负责听到敲门声（新连接），开门（`accept`），然后将来访者（新 socket）引荐给主人（`TcpServer`）。

---

#### 一、类的成员变量概览（来自头文件，在实现中可见）

虽然头文件未在给出的代码中展示，但从实现可以推断出其成员变量：

- **`EventLoop* loop_;`**：所属的事件循环（通常就是 `baseLoop_`，即主 Reactor）。
- **`int acceptSocket_;`**：监听 socket 的文件描述符。
- **`std::unique_ptr<Channel> acceptChannel_;`**：用于监听 `acceptSocket_` 读事件的 Channel。
- **`bool listening_;`**：标记是否已开始监听。
- **`NewConnectionCallback newConnectionCallback_;`**：上层设置的回调，用于处理新连接。

---

#### 二、构造函数 `Acceptor::Acceptor(EventLoop* loop, const sockaddr_in& listenAddress)`

```cpp
Acceptor::Acceptor(EventLoop* loop, const sockaddr_in& listenAddress)
    : loop_(loop),
      acceptSocket_(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),
      acceptChannel_(nullptr),
      listening_(false) {
    // 1. 创建 socket
    if (acceptSocket_ < 0) throwSocketError("socket");

    // 2. 设置 SO_REUSEADDR（端口复用）
    int enabled = 1;
    if (::setsockopt(acceptSocket_, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)) < 0) {
        ::close(acceptSocket_);
        throwSocketError("setsockopt(SO_REUSEADDR)");
    }

    // 3. 绑定地址
    if (::bind(acceptSocket_, reinterpret_cast<const sockaddr*>(&listenAddress), sizeof(listenAddress)) < 0) {
        ::close(acceptSocket_);
        throwSocketError("bind");
    }

    // 4. 创建 Channel 并设置读回调
    acceptChannel_ = std::make_unique<Channel>(loop_, acceptSocket_);
    acceptChannel_->setReadCallback([this] { handleRead(); });
}
```

- **作用**：创建一个监听 socket，绑定到指定地址，并准备好接受连接。
- **流程**：
  1. **创建 socket**：使用 `socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)`，直接获得 **非阻塞** 和 **CLOEXEC** 属性的 socket，无需额外设置。
  2. **设置 `SO_REUSEADDR`**：允许端口重用，防止服务器重启时因 `TIME_WAIT` 而绑定失败。
  3. **绑定（`bind`）**：将 socket 绑定到传入的 `listenAddress`。
  4. **创建 `Channel`**：为 `acceptSocket_` 创建一个 `Channel`，并设置其读回调为 `Acceptor::handleRead`。**注意**：此时并未开启读事件监听（即未调用 `enableReading`），因为还未调用 `listen`。
- **错误处理**：任何一步失败，都会关闭已打开的资源并抛出异常。

---

#### 三、析构函数 `Acceptor::~Acceptor()`

```cpp
Acceptor::~Acceptor() {
    loop_->assertInLoopThread();
    if (listening_) {
        acceptChannel_->disableAll();
        acceptChannel_->remove();
    }
    ::close(acceptSocket_);
}
```

- **作用**：清理资源。
- **流程**：
  1. 断言当前线程是所属的 `EventLoop` 线程（防止跨线程销毁）。
  2. 如果正在监听（`listening_ == true`），先关闭 Channel 的所有事件（`disableAll`），并将其从 `Poller` 中移除（`remove`）。
  3. 关闭 `acceptSocket_`。

---

#### 四、设置新连接回调 `void Acceptor::setNewConnectionCallback(NewConnectionCallback callback)`

```cpp
void Acceptor::setNewConnectionCallback(NewConnectionCallback callback) {
    newConnectionCallback_ = std::move(callback);
}
```

- **作用**：由上层（`TcpServer`）设置回调函数，当有新连接时会被调用。
- **参数 `callback`**：通常是一个接受 `(int socketFd, const sockaddr_in& peerAddress)` 的函数，其中 `socketFd` 是新建立的客户端 socket，`peerAddress` 是客户端地址。

---

#### 五、开始监听 `void Acceptor::listen()`

```cpp
void Acceptor::listen() {
    loop_->assertInLoopThread();
    if (listening_) return;

    if (::listen(acceptSocket_, SOMAXCONN) < 0) {
        throwSocketError("listen");
    }

    listening_ = true;
    acceptChannel_->enableReading();
}
```

- **作用**：使监听 socket 进入 `LISTEN` 状态，并开始监听读事件（即新连接到达事件）。
- **流程**：
  1. 断言在所属线程。
  2. 如果已经在监听，直接返回（幂等）。
  3. 调用 `listen`，`SOMAXCONN` 表示使用系统最大积压队列长度。
  4. 设置 `listening_ = true`。
  5. 调用 `acceptChannel_->enableReading()`，使得当有连接到达时，`epoll` 会触发 `acceptChannel_` 的读事件，从而调用 `handleRead`。

---

#### 六、查询状态 `bool Acceptor::listening() const noexcept`

- **作用**：返回 `listening_` 标志，供外部查询是否已开始监听。

---

#### 七、核心事件处理函数 `void Acceptor::handleRead()`

这是 **最核心的函数**，它会在监听 socket 上有新连接时被调用（由 `Channel` 的读回调触发）。

```cpp
void Acceptor::handleRead() {
    loop_->assertInLoopThread();

    while (true) {
        sockaddr_in peerAddress{};
        socklen_t addressLength = sizeof(peerAddress);

        const int socketFd = ::accept4(
            acceptSocket_,
            reinterpret_cast<sockaddr*>(&peerAddress),
            &addressLength,
            SOCK_NONBLOCK | SOCK_CLOEXEC);

        if (socketFd >= 0) {
            if (newConnectionCallback_) {
                newConnectionCallback_(socketFd, peerAddress);
            } else {
                ::close(socketFd);
            }
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        throwSocketError("accept4");
    }
}
```

- **作用**：**接受所有当前等待的新连接**。使用 `while` 循环一次将所有积压的连接全部 accept，避免因只处理一个而漏掉其他连接。
- **关键点**：
  - 使用 `accept4`（而非 `accept`），直接设置新 socket 为 **非阻塞** 和 **CLOEXEC**，无需额外 `fcntl`。
  - **循环条件**：
    - 成功获取 `socketFd`：调用 `newConnectionCallback_` 将新连接传递给上层（如果回调未设置，则直接关闭该 socket）。
    - 继续尝试 accept 下一个。
  - 如果 `accept4` 返回 -1：
    - **`EAGAIN` / `EWOULDBLOCK`**：表示当前没有更多等待的连接，此时退出循环。
    - **`EINTR`**：系统调用被信号中断，重试。
    - 其他错误：抛出异常。
- **为什么需要 `while`**：因为 `EPOLLIN` 事件可能在一次触发时已有多个连接到达，一次性全部 accept 可减少系统调用和事件循环次数。

---

#### 八、设计亮点总结

| 特性                  | 实现方式                                    | 优势                                         |
| :-------------------- | :------------------------------------------ | :------------------------------------------- |
| **非阻塞监听**        | `socket` 直接指定 `SOCK_NONBLOCK`           | 监听 socket 本身非阻塞，避免 `accept` 阻塞。 |
| **`accept4` 优化**    | 一次调用完成 accept + 设置非阻塞            | 减少系统调用（无需额外 `fcntl`）。           |
| **一次性循环 accept** | `while` 循环直到 `EAGAIN`                   | 减少 `epoll` 触发次数，提高吞吐。            |
| **回调解耦**          | 通过 `newConnectionCallback_` 传递新 socket | 使 `Acceptor` 不关心业务，只负责接受连接。   |
| **异常安全**          | 失败时关闭资源并抛异常                      | 保证构造失败时不会留下悬空资源。             |

---

#### 九、使用场景（与 `TcpServer` 的配合）

`TcpServer` 在启动时创建 `Acceptor`，并设置 `setNewConnectionCallback`，回调内部会：

1. 从 `EventLoopThreadPool` 中获取一个子 Reactor（`EventLoop*`）。
2. 创建 `TcpConnection` 对象，将其绑定到该子 Reactor 的 `Channel` 上。
3. 调用 `connectionCallback` 通知用户。

**Acceptor 的职责到此结束**——它只负责“敲门开门”，后续的读写、关闭都由 `TcpConnection` 处理。

---

**一句话总结**：`Acceptor` 是 mini muduo 的 **“连接受理器”**，它封装了监听 socket 的创建、绑定、监听和接受连接的全部流程，并将新连接通过回调安全地交给上层，是 `TcpServer` 实现“主从 Reactor”模型的基础组件。😊



#### TlsContext



























# question

1.   NonCopyable中，使用protected有什么用？整体有什么作用？
2.   
