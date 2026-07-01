# MIO~



## FTP

### setsockopt 是什么？

全称：

```
set socket option
```

用于修改 Socket 的行为。

函数原型：

```
int setsockopt(
    int sockfd,
    int level,
    int optname,
    const void *optval,//传递选项值。
    socklen_t optlen
);
```

成功返回：

```
0
```

失败返回：

```
-1
```

因此代码里：

```
setsockopt(...) < 0
```

表示：

```
如果设置失败
```

所以说：

```
int opt = 1;

if (setsockopt(
        serverFd,//服务器监听 Socket。
        SOL_SOCKET,//在 Socket 层设置选项
        SO_REUSEADDR,//允许重用本地地址
        &opt,//传递选项值，开启 SO_REUSEADDR
        sizeof(opt)) < 0)//opt 占多少字节
{
    perror("setsockopt");
    exit(EXIT_FAILURE);
}
```

网络协议栈有很多层：这里告诉系统：

```
我要修改 Socket 自身的选项
```

```
应用层
↑
TCP层
↑
IP层
↑
Socket层
```

第三个参数 SO_REUSEADDR

最重要。

```
SO_REUSEADDR
```

意思：

```
允许重用本地地址
```

------

#### 没有它会发生什么？

假设服务器：

```
bind(8080)
listen()
```

运行中。

然后：

```
Ctrl+C
```

关闭服务器。

这时内核不会立刻释放端口。

TCP 会进入：

```
TIME_WAIT
```

状态。

可能持续几十秒。

此时马上重启：

```
./server
```

会出现：

```
bind: Address already in use
```

即：

```
端口已被占用
```

实际上是上一个连接还没彻底清理。

------

#### 开启 SO_REUSEADDR

```
SO_REUSEADDR = 1
```

后：

```
bind()
```

可以立即重新绑定端口。

开发服务器几乎都会写：

```
int opt = 1;

setsockopt(
    serverFd,
    SOL_SOCKET,
    SO_REUSEADDR,
    &opt,
    sizeof(opt));
```

**请给 serverFd 这个 Socket 设置 SO_REUSEADDR 选项，并把它设置为开启状态。**



### sockaddr_in 是什么

socket address internet  IPv4 网络地址

```
struct sockaddr_in
{
    sa_family_t sin_family; // 地址族

    in_port_t sin_port;     // 端口号

    struct in_addr sin_addr;// IP地址

    char sin_zero[8];
};
```

所以说





```
为什么后面有 {}
sockaddr_in serverAddr {};

这是 C++11 的统一初始化。

作用：

全部清零

等价于：

sockaddr_in serverAddr;
memset(&serverAddr, 0, sizeof(serverAddr));

因此：

sin_family = 0
sin_port   = 0
sin_addr   = 0

先初始化干净。
```

```
serverAddr 的类型

前面定义的是：

sockaddr_in serverAddr;

类型：

sockaddr_in*
```



###  sin_addr.s_addr

```
serverAddr.sin_addr.s_addr

表示：

IP地址

例如：

127.0.0.1
192.168.1.100
8.8.8.8

都存在这里。
```

#### INADDR_ANY

```
代码：
serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

其中：

INADDR_ANY

值实际上：

0

即：

0.0.0.0

含义不是：

监听 0.0.0.0

而是：

监听本机所有IP

例如机器有：

127.0.0.1
192.168.1.100
10.0.0.5

那么：

INADDR_ANY

等价于：

全部监听

客户端都能连：

127.0.0.1:21
192.168.1.100:21
10.0.0.5:21

如果写：

inet_addr("127.0.0.1")

则只能本机访问。
```

### sin_port

```
serverAddr.sin_port = htons(FTP_PORT);

表示：

监听端口

例如：

FTP_PORT = 21

则：

监听21端口
```

```
htons()

全称：

host to network short

因为端口是：

unsigned short

所以使用：

htons()

而不是：

htonl()
```



### bind

```
int bind(
    int sockfd,
    const struct sockaddr *addr,
    socklen_t addrlen
);
```

```
第二个参数要求：

sockaddr*

因此：

sockaddr_in*

需要转换成：

sockaddr*

所以写：

reinterpret_cast<sockaddr*>(&serverAddr)
```

```
返回值

成功：

bind(...) == 0

失败：

bind(...) == -1

所以：

if (bind(...) < 0)

表示：

绑定失败
```

```
常见错误
端口已占用
bind: Address already in use

例如：

已有程序监听 21 端口
权限不足
bind: Permission denied

Linux 下：

1024 以下端口

通常需要管理员权限。

例如：

21
22
80
443
```

```
如果绑定失败：

close(serverFd);

释放 Socket。

否则会造成资源泄漏。
```



### listen

```
int listen(
    int sockfd,//就是之前创建并绑定好的 Socket。
    int backlog///等待队列长度(backlog)
);
```

```
什么叫等待队列？

假设服务器正在工作：

客户端A ---->
客户端B ---->
客户端C ---->
客户端D ---->
客户端E ---->
客户端F ---->

而服务器暂时来不及处理。

内核会把这些连接请求先放进队列。

┌─────┐
│  A  │
├─────┤
│  B  │
├─────┤
│  C  │
├─────┤
│  D  │
├─────┤
│  E  │
└─────┘

队列长度：

5

表示：

最多缓存约5个等待处理的连接

如果第 6 个客户端来：

F

可能会：

连接失败

或者：

被拒绝

具体行为由操作系统决定。
```

```
listen(serverFd, SOMAXCONN);

表示：

使用系统允许的最大队列长度

更合理。
```

```
状态变成：

LISTEN

这时候：

客户端可以连接

例如：

ftp 127.0.0.1 21

或者：

telnet 127.0.0.1 21

都能发起 TCP 三次握手。
```

```
返回值？

成功：

listen(...) == 0

失败：

listen(...) == -1

因此：

if (listen(...) < 0)

表示：

监听失败
```



### accept

```
int accept(
    int sockfd,//监听 Socket。
    struct sockaddr *addr,//客户端地址信息
    socklen_t *addrlen
);
```



```
连接成功后：

clientAddr.sin_addr

保存客户端 IP。

clientAddr.sin_port

保存客户端端口。
```

```
第三个参数
&clientLen

输入：

结构体大小

输出：

实际写入大小
```



```
accept 会阻塞

这是非常重要的概念。

当没有客户端时：

accept(...)

会停在这里。

等待...
等待...
等待...

不会继续执行。

例如：

std::cout << "before\n";

accept(...);

std::cout << "after\n";

运行结果：

before

然后卡住。

直到客户端连接。

才会输出：

after
```

```
实际上：

serverFd

负责：

监听
clientFd

负责：

通信

例如：

int serverFd = socket(...);

bind(...);

listen(...);

得到：

监听Socket

客户端连接：

int clientFd = accept(...);

得到：

通信Socket

关系如下：

serverFd
    ↓
监听21端口

客户端A：

accept()
   ↓
clientFd = 5

客户端B：

accept()
   ↓
clientFd = 6

客户端C：

accept()
   ↓
clientFd = 7

所以：

serverFd 永远不变

负责接客。

而：

clientFd

负责和某个具体客户端聊天。
```



### INET_ADDRSTRLEN 是什么？

系统定义的常量：

```
INET_ADDRSTRLEN
```

IPv4 下通常是：

```
16
```

因为最长的 IPv4 地址：

```
255.255.255.255
```

长度：

```
15 个字符
```

再加：

```
\0
```

结束符。

所以：

```
char ip[16];
```

实际上和：

```
char ip[INET_ADDRSTRLEN];
```

效果一样。

### inet_ntop

network to presentation
网络格式 → 可读格式

```
inet_ntop(
    AF_INET,//ipv4
    &clientAddr.sin_addr,//客户端ip   字符串连接过来是二进制格式
    ip,//转换后存在这里，是十进制模式
    sizeof(ip));//最多写多少字节
```

```
const char* inet_ntop(
    int af,
    const void* src,
    char* dst,
    socklen_t size
);
```



### sendReply

```
创建字符串流  输出到字符串。
std::ostringstream oss;

例如：

oss << "hello";

最后：

oss.str()

得到：

hello
```

```
转换成字符串
std::string msg = oss.str();
```



### 220 是什么？

FTP 协议规定服务器回复时必须带状态码。

例如：

```
220 Service ready
331 Need password
230 Login successful
221 Goodbye
```

这些数字和 HTTP 状态码有点类似：

```
HTTP
200 OK
404 Not Found

FTP
220 Ready
230 Login successful
530 Login incorrect
```



### recv

```
recv(clientFd, &ch, 1, 0)//每次只请求 1 个字节，存储在 ch 中
```

这行代码调用 `recv` 函数，**从已连接的 TCP 套接字 `clientFd` 中接收 1 个字节的数据**，并将其存储到字符变量 `ch` 的地址中。

### std::string::npos



`std::string::npos` 是 C++ 标准库中 `std::string` 类定义的一个**静态常量**，通常用于表示“未找到”、“直到字符串末尾”或“无效位置”。

`std::string::npos` 是一个 `size_t` 类型（无符号整数），其值通常是 **`size_t(-1)`**（即所有位都为 1，在 64 位系统上是 `18446744073709551615`）。

这样设计的目的是：因为任何有效的索引（0,1,2,...）都不可能等于这个最大值，所以可以用它作为“非法位置”的标记。

### fs::weakly_canonical(path)

-   **作用**：将路径规范化（移除 `.`、`..`、解析符号链接，但不要求路径存在）。
-   **为什么用 `weakly_canonical` 而不是 `canonical`？**
    -   `canonical` 要求路径必须存在，否则抛异常。
    -   对于 **上传新文件** 的路径（还不存在），`canonical` 会失败，而 `weakly_canonical` 可以处理不存在的路径（尽可能规范化，剩余部分保留）。
-   **举例**：
    `path = "/home/ftp_root/../etc/passwd"` → `weakly_canonical` 返回 `"/home/etc/passwd"`（假设 `/home/ftp_root/..` 规范化后是 `/home`）



### PASV

一、为什么需要 PASV？

FTP 使用两个独立的 TCP 连接：

-   **控制连接**（端口 21）：发送命令和接收回复。
-   **数据连接**：传输目录列表、文件内容等。

在 **主动模式（PORT）** 中，服务器主动连接客户端的一个随机端口，但现代客户端常常位于防火墙或 NAT 后面，无法直接被服务器连接。
**被动模式** 让服务器打开一个临时端口，**客户端主动连接** 这个端口，从而绕过防火墙限制。

二、PASV 的工作流程

1.  客户端发送 `PASV` 命令。
2.  服务器开启一个临时监听 socket（例如端口 50000）。
3.  服务器回复 `227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)`，其中：
    -   `h1,h2,h3,h4` 是服务器的 IP 地址（逗号分隔）。
    -   `p1 = 端口号 / 256`，`p2 = 端口号 % 256`。
4.  客户端根据该 IP 和端口，主动发起 TCP 连接。
5.  服务器接受连接，得到数据 socket，然后进行数据传输。

三、典型示例

plaintext

```
C: PASV
S: 227 Entering Passive Mode (192,168,1,100,195,128)
                IP = 192.168.1.100
                端口 = 195*256 + 128 = 50048
C: LIST
S: 150 ...
[客户端主动连接 192.168.1.100:50048]
[服务器通过该数据连接发送目录列表]
S: 226 Transfer complete
```

四、与主动模式（PORT）的区别

| 模式             | 数据连接发起方   | 适用场景                               |
| :--------------- | :--------------- | :------------------------------------- |
| **PORT（主动）** | 服务器连接客户端 | 客户端没有防火墙/NAT，或可配置端口转发 |
| **PASV（被动）** | 客户端连接服务器 | 客户端位于防火墙/NAT 后面（最常见）    |

绝大多数现代 FTP 客户端默认使用 **PASV** 模式。

五、总结

-   **PASV** 是 FTP 被动模式的命令，告诉服务器“请开放一个临时端口，我来连接你”。
-   它解决了客户端无法被直接访问时的数据传输问题。
-   响应中给出的端口信息需要客户端解析并主动建立数据连接。

在你的 FTP 服务器代码中，`handlePASV` 函数就是实现这个逻辑的地方：创建监听 socket，回复 `227`，等待后续数据命令调用 `acceptDataConnection`。















## 了

### 为什么不能直接保存entry->d_name?

entry是一个指针，是linux定义好的结构体

```
struct dirent*entry;
```

里面存储inode,d_name

```
entry = readdir(dir);
```

从目录读一个文件，把文件信息放进struct dirent里面，返回一个指针entry

它的内存是系统复用的，只有一块内存，每次调用的readdir会直接覆盖上一次的内容

，所以每次要先拷贝到自己的内存里面，防止之前读到的被覆盖掉

```
files[*count].name = strdup(entry->d_name);
```

### 为什么一定要生成完整路径？

因为readdir只返回文件名字，但是完整路径是其他方法也要用的

### 学会使valgrind

可以检查内存是否泄漏

编译

```
gcc -g -Wall -Wextra myls.c -o myls
```

-g就是给valgrind使用的，生成调试信息，让程序可以使用gdb调试器调试

gcc是一个**执行编译的工具**

`-Wall`开启所有基础警告，检查潜在bug

`-Wextra`开启额外严格警告，比wall更加严格

myls.c是源代码文件

-o myls 指定**输出的可执行文件名为 myls** 不加这个参数，默认生成 `a.out`

检查泄漏

```
valgrind --leak-check=full ./myls -is
```

### &位运算

看起来功能好像差不多，但是

&&是条件判断

&1.判断文件权限，奇偶盘判断x&1,清零某些二进制位

### st_time不能直接打印

他是时间戳，必须格式化之后人

### void func(void)

意思就是不要传入任何参数

**函数****不接收任何参数**

= 调用时不能传值

= 必须写成 `func();`

### 打印内存地址（% p）

**`void \*)`**

强制转为通用空指针类型

`(void *)` 是为了符合 `printf` 打印地址的标准写法。

```
 printf("Text/code function address  = %p\n", (void *)main);
    printf("global_init address         = %p\n", (void *)&global_init);
    printf("global_uninit address       = %p\n", (void *)&global_uninit);
    printf("static_init address         = %p\n", (void *)&static_init);
    printf("static_uninit address       = %p\n", (void *)&static_uninit);
    printf("heap_var address            = %p\n", (void *)heap_var);
    printf("local_main address          = %p\n", (void *)&local_main);
```



### 命令行参数 & 环境变量

✅ **argv 和 envp 都存放在 栈（stack） 的最顶端**

✅ **它们是进程启动时，系统自动压入栈的**

✅ **地址比你函数里的局部变量更高！**

内存位置总结（从低到高）

1. **代码段**（.text）→ main / 函数地址
2. **数据段**（.data）→ 全局 / 静态初始化变量
3. **BSS** → 全局 / 静态未初始化
4. **堆**（heap）→ malloc
5. **栈**（stack）→ 局部变量
6. **栈顶最高处** → **argv、envp**（你打印的这几个）

argv、envp 是栈顶的特殊数据，存放命令行参数和环境变量，地址最高！

### -O0

O0 = 优化等级 0 = 不做任何优化

- 如果你开优化（`-O1`/`-O2`），编译器会**修改变量位置、删掉没用的代码**
- 你的**内存地址打印就会不准、甚至乱掉**
- **`-O0` 保证内存布局 100% 真实、原样**

你在做**内存布局实验**，**必须加 `-O0`**！

**`-O0` = 禁止优化，保证内存地址真实**static jmp_buf env;

### static jmp_buf env;

`jmp_buf`

- 是一个**数据类型**（数组 / 结构体）
- **专门用来保存：CPU 寄存器 + 栈状态**
- 作用：**记录 “跳回哪里”**

`env`

- 变量名，你可以随便改（`buf`/`context` 都行）
- 用来**存储跳转环境**

### `fprintf`

- 作用：**格式化输出到指定文件流**（区别于 `printf` 只能输出到屏幕）
- 语法：`fprintf(文件流, 格式化字符串, 参数...)`

### static volatile sig_atomic_t stop = 0;

**`volatile`**：告诉编译器不要优化这个变量，保证信号 handler 里修改后，主循环能立刻读到最新值

告诉编译器这个变量可能被异步改变，不要错误优化。

**`sig_atomic_t`**：C 标准保证**读写操作不可中断**，是信号处理中唯一安全的整型类型

读写这个类型适合在信号处理器和主程序之间共享；

这是**跨线程 / 信号通信**的唯一安全方式

### socket

通信的端点

### 异步

也就是说，进程不知道什么时候会收到信号。

程序可能正在执行：

```c
x = x + 1;
```

突然信号来了，操作系统就打断当前执行，转去执行信号处理器。

### write(STDOUT_FILENO, "SIGINT handled\n", 15);

write是系统调用，底层往文件描述符写入数据，信号处理函数只能使用它，不能使用printf

**STDOUT_FILENO**

标准输出文件描述符，等价数字 **1**，代表打印到终端屏幕。

**"SIGINT handled\n"**

要输出的字符串：

**15**

要写入的**字节个数**

### sigemptyset

清空这个盒子，里面没有信号

### sa.sa_mask

当信号处理函数正在运行时，暂时屏蔽掉其他信号

sigemptyset(&sa.sa_mask);

在执行信号处理函数 handler 时，**不额外屏蔽任何信号**。

谁来都能打断我，我不拦着



你正在接电话（= 执行信号处理函数）

- `sa.sa_mask` = 你设置的 **“免打扰名单”**
- `sigemptyset(&sa.sa_mask)` = **清空免打扰名单 → 不设置任何免打扰**

所以：

- 你接电话时，**别人可以继续打进来**
- 信号处理函数运行时，**其他信号可以正常触发**

### -O2 

`-O2`

**二级优化**

让程序运行**更快、体积更小**

（写练习、刷题都推荐加）

### waitpid

```
waitpid(pid, NULL, 0);
```

**pid**

要等的**子进程 ID**（就是你 fork 完得到的那个变量）

**NULL**

不关心子进程的退出状态（填 NULL 最简单）

**0**

表示：**一直等，等到死为止（阻塞等待）**

### lseek

获取当前文件偏移量

`lseek(fd, 0, SEEK_CUR)`

`fd`：文件描述符

`0`：偏移增量，**不动位置**

`SEEK_CUR`：以**当前光标位置**为基准





`SEEK_SET`：从**文件开头**算

`SEEK_CUR`：从**当前位置**算

`SEEK_END`：从**文件末尾**算

### setbuf

`setbuf(stdout, NULL);`

关闭输出缓冲区，让 printf 立刻打印到屏幕，不缓存！

如果**不加**这句：

- `printf` 不会马上打印
- 数据先存在**缓冲区**里
- `fork` 一创建子进程，**缓冲区会被复制给子进程**
- 结果就是：**同一句话打印两次，输出乱套**

加了之后：

- `printf` **立即输出**，不留缓存
- 父子进程各自输出不会乱
- 你看到的结果才是**干净、正确、不重复**的

### usleep

让程序 “睡一会儿”，暂停 0.1 秒，再继续跑

- **usleep** = 微秒级睡眠
- **1 秒 = 1,000,000 微秒**
- **100000 微秒 = 0.1 秒**

### sync_pipe

Synchronization Pipe

**让子进程先干完，父进程再开始**，比 `sleep/usleep` 更精准、更专业！

因为 waitpid 只能等子进程 “整个结束”，

而 sync_pipe 可以让子进程 “没结束” 就通知父进程！

✅ waitpid(pid, NULL, 0)

**父进程等：子进程彻底退出、死掉、消失**

才能继续运行。

✅ sync_pipe（管道同步）

**父进程等：子进程完成某一步操作**

子进程不用死，还能继续跑！

### **sigprocmask**

：设置信号屏蔽

sigprocmask(SIG_BLOCK, &block_set, &old_set)

signal process mas

信号进程屏蔽字:哪些信号可以打断进程，哪些信号要暂时 “拦住、延后处理”

```
sigprocmask(操作方式, 新信号集, 保存旧信号集);
```

**SIG_BLOCK**

→ **屏蔽**（加上这些信号）

**SIG_UNBLOCK**

→ **放开屏蔽**（允许这些信号来了）

**SIG_SETMASK**

→ **直接设置新的屏蔽规则**





为什么要用它？（你的场景）

你在做**父子进程同步**：

- 你**不希望信号突然打断关键代码**
- 所以先**屏蔽信号**
- 等准备好后，再**放开、接收信号**

### `old_set` 

- 类型：`sigset_t`
- 作用：**用来保存【修改前的】信号屏蔽字**

它的作用：**备份！**

- 你现在屏蔽了 `SIGUSR1`
- 但你**不能一直屏蔽**
- 所以要先**保存旧的设置**
- 用完后**恢复回去**

### sigsuspend(&old_set)

1. **临时恢复信号屏蔽字**（让 SIGUSR1 可以被收到）
2. **进程挂起，休眠等待信号**
3. **信号来了 → 唤醒，恢复之前的屏蔽状态**

### `atexit(函数名)`：

**把函数注册到「程序正常退出执行列表」**

程序调用 `exit()` 正常退出时，自动执行这些注册函数

注册失败返回非 0，这里做错误判断

`atexit` 遵循**栈规则：先进后出**

### getaddrinfo()

之前学的

```
struct sockaddr_in addr;

addr.sin_family = AF_INET;
addr.sin_port = htons(9090);

inet_pton(AF_INET,
          "127.0.0.1",
          &addr.sin_addr);
```

### Socket:Internet domain

#### getaddrinfo()

为什么使用它？

因为在此之前，一步一步都需要自己设定，创建结构体，设置ipv4，设置端口，自己调用

```
struct sockaddr_in addr;

addr.sin_family = AF_INET;
addr.sin_port = htons(9090);
//IP地址转换函数：十进制字符串转换到二进制网络字节序
inet_pton(AF_INET,//地址族
          "127.0.0.1",//十进制字符串
          &addr.sin_addr);//转换后二进制存储位置
```

假如说现在是ipv4,过段时间变成ipv6,又需要改变代码，这时候不如直接使用getaddrinfo

```
struct addrinfo hints;//要求

struct addrinfo *res;//系统返回的结果，是一个链表，因为一个域名可能有很多各IP地址，为了避免负载过大，有多台服务器共同等待，每一个节点里面都有
{rp->ai_family,地址类型。
rp->ai_socktype,TCP还是UDP。
rp->ai_addr真正的地址结构。}

memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;      // IPv4 或 IPv6 都可以，AF_UNSPEC：无所谓 你帮我找


hints.ai_socktype = SOCK_STREAM;  // 要多是TCP地址，SOCK_DGRAM这个是UDP地址

int ret = getaddrinfo("127.0.0.1", "9090", &hints, &res);//IP,(格式无要求)端口，（数字），要求，结果
if (ret != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    exit(1);
}

// 使用 res 链表

freeaddrinfo(res);
```

#### getnameinfo()

如果说getaddrinfo是把ip和端口绑定在socket上面，那么getnameinfo()就是把sockaddr变成ip+端口字符串

```
char host[NI_MAXHOST];
char service[NI_MAXSERV];

getnameinfo((struct sockaddr *)&addr//输入, addrlen,//地址结构长度
            host//IP地址, sizeof(host),//缓冲区大小
            service//端口, sizeof(service),//缓冲区长度
            NI_NUMERICHOST//直接返回数字IP | NI_NUMERICSERV);//直接返回数字端口
```

二进制IP和网络字节序端口人看不懂，所以需要转换称十进制`192.168.1.100`，还有端口号`54321`，就可以看懂了

#### 应用层协议

TCP是字节流。

你学过：

```
一次write
≠
一次read
```

------

例如客户端：

```
write(fd,"hello",5);
write(fd,"world",5);
```

------

服务器可能一次收到：

```
helloworld
```

------

也可能：

```
he
llo
wo
rld
```

------

服务器根本不知道：

```
哪里是一条消息的结束
```

------

所以必须自己规定规则。

这就叫：

```
应用层协议
```

### 
