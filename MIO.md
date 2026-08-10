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