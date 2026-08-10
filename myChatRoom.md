[TOC]

# myChatRoom

## eeeee



#### DAY1

##### 广播是什么意思呢？

是聊天室最基本的功能就是说客户端发一句话之后，服务端给所有客户端发送这条消息

与之对应的还有：

-   单播（Unicast）：发给一个客户端
-   广播（Broadcast）：发给所有客户端
-   组播（Multicast）：发给某个群组

##### CMAKE,Bazel,Ninja是啥？

| 特性         | **CMake**                                                    | **Ninja**                                             | **Bazel**                                                    |
| :----------- | :----------------------------------------------------------- | :---------------------------------------------------- | :----------------------------------------------------------- |
| **核心定位** | **构建系统生成器**                                           | **高性能构建系统**                                    | **构建与测试工具**                                           |
| **主要职责** | 读取`CMakeLists.txt`，生成平台原生的构建文件（如Makefile、Ninja文件、Visual Studio工程等） | 执行`.ninja`构建文件，以**极快的速度**编译和链接代码  | 读取`BUILD`文件，**独立完成**从依赖管理到编译、测试、打包的全流程 |
| **工作方式** | **配置 + 生成**：先配置，再交给其他工具去构建                | **执行**：只负责运行构建命令，不做高层决策            | **一体化**：集成了构建、测试、依赖管理、缓存等               |
| **复杂度**   | 中等，脚本语言（`CMakeLists.txt`）功能强大但语法独特         | **低**，构建文件`build.ninja`语法简单，通常不手动编写 | **高**，概念和配置（`BUILD`、`WORKSPACE`）复杂，学习曲线陡峭 |
| **依赖管理** | **弱**，依赖库通常需预先安装在系统上，通过`find_package`查找 | **无**，它只负责构建，不管理依赖                      | **强**，内置依赖管理，可直接从网络获取依赖                   |
| **典型场景** | **跨平台项目的事实标准**，适用于绝大多数C/C++项目            | 作为**CMake的后端**，用于加速**大型项目**的构建       | **超大规模、多语言项目**（如Google的TensorFlow），对构建速度和一致性要求极高 |



>   CMAKE就是在不同的操作系统上面生成构建文件的，读取CMakeLists.txt,检测系统然后生成本地化的构建文件

-   Windows上可能是 **Visual Studio 解决方案（.sln）**
-   Linux/macOS上可能是 **Makefile**
-   也可以是 **Ninja 构建文件（[build.ninja](https://build.ninja/)）**

>   Ninja是一个负责驱动编译器去编译和链接的，按照cmake生成的build.ninja问价内指令，高效并行调用gcc来编译.cc生成.o，再调用链接器生成可执行文件

Ninja的设计哲学是“**快**”。它通过精简设计、高效的任务调度和最小化磁盘I/O来达到极致的构建速度，尤其擅长**增量编译**。

>   Make和ninja的功能其实是一样的，都是先使用Cmake生横makefile然后敲下make命令，Make开始干活
>
>   Make：功能强大，语法灵活，可以直接手写makefile,单核编译可以，但是大型项目的增量编译时较慢
>
>   Ninja：极简主义，语法固定，追求机制的编译速度，专门为增量编译优化，并行调度很快，但是生成的build.ninja人类看不懂，所以需要cmake生成

Bazel是Google开源的一体化构建工具，旨在解决超大型、多语言、多仓库项目（monorepo）的构建问题。

---

**为什么需要ninja或者make?**

1.   因为如果调用了很多第三方库，命令太长而且容易输入错误，缺失，makefile直接被保存下来，只需要输入`make`
2.   如果修改了一个文件，所有都要全部重新编译，会消耗时间，但是make/ninja机制是增量编译，会检查文件的修改时间戳，当输入`make`时，只会编译今天修改的那个文件，生成新的.o文件，剩下的其他没有修改的文件直接拿来链接，时间大大缩短
3.   如果说有一个头文件改变了，那么就要编译所有引用这个头文件的文件，但是有make/ninja就会自动解析依赖图，发现头文件变了之后自动重新编译所有引用的文件
4.   如果电脑是16核的，那么g++一次性只能编译一个文件，但是cmake/ninja会同时启动16个g++进程，把文件分给16个核心同时编译，cpu拉满，速度很快



**cmake**

当输入`cmake ..`时计算机做三件事：

​       1.读取说明书：读取根目录下的CMakeLists.txt文件，里面又我要生成什么执行文件，需要哪些依赖库，编译参数是什么

​       2.检测环境：自偶都能够检查电脑系统，编译器版本，第三方库是否安装

​       3.生成图纸：根据检测结果，生成当前开发环境的构建文件

构建文件里面有什么？

​        取决于操作系统和编译器：

​                在linux/mac里面生成makefile,之后输入make就会根据这个文件编译

​                 在windows使用visualstudio,生成.sln（  解决方案文件）和.vcxproj(项目文件)，之后双击这个解决方案就可以在vs里面一键编译

##### std::stoi

是标准库string中提供的**字符串转整数函数**，在网络编程中经常被用来解析客户端发的命令，或者是将接受到的数字字符串比如说是端口号和用户id转为整形

```c++
#include <string>
#include <iostream>
        //        要转换的字符串 存储第一个未转换字符的位置  进制，默认十进制，也可以是                                                                   十六进制等等 
int stoi(const std::string& str, std::size_t* pos = 0, int base = 10);

int main() {
    std::string s1 = "42";
    int num = std::stoi(s1);  // num = 42
    std::cout << num << std::endl;
}
```

`std::stoi` 在转换失败时会**抛出异常**。如果不捕获，程序会直接终止（core dump / 闪退）。对于服务端程序，一个恶意或错误的数据包绝不能让整个服务器挂掉。

它会抛出两种主要异常：

-   **`std::invalid_argument`**：字符串中**没有**可转换的数字（如 `"abc"`、空字符串 `""`）。
-   **`std::out_of_range`**：数字超出了 `int` 的表示范围（如 `"999999999999"`）。



##### 缓冲区Buffer

本质上都遵循同一个核心目的：**解决“速度不匹配”或“数据大小不匹配”的问题**，充当一个“中间暂存区”。

| 层级                | 典型代表                                       | 核心作用                                           | 生活中的比喻                                               |
| :------------------ | :--------------------------------------------- | :------------------------------------------------- | :--------------------------------------------------------- |
| **硬件层**          | CPU缓存、帧缓冲区（显存）                      | 匹配CPU与内存/硬盘/显示器的**速度鸿沟**            | 流水线上的**操作台**，工人（CPU）把零件放上去快速加工      |
| **操作系统内核层**  | 页缓存（Page Cache）、Socket缓冲区、管道缓冲区 | 减少对慢速磁盘/网卡的频繁读写，合并IO操作          | 快递公司的**中转仓库**，批量收发包裹，不用每件都跑一趟总仓 |
| **运行时/标准库层** | C语言的`stdio`缓冲区、C++的`iostream`缓冲区    | 将多次系统调用合并为少数几次，**提升用户态IO性能** | 餐厅的**传菜口**，服务员攒够几道菜再一起端出去             |
| **应用/业务层**     | 双缓冲（绘图）、日志缓冲区、Netty的ByteBuf     | 满足业务逻辑需求，如**防止画面撕裂**、**异步解耦** | 电影院的**AB双幕布**，放A幕时在B幕准备下一帧，无缝切换     |

---



🔍 深入拆解几个关键缓冲区

###### 1. 硬件底层：解决速度差

-   **CPU缓存（L1/L2/L3）**：CPU运算速度比内存快几十倍，缓存充当CPU和内存之间的“极速便签”，把常用数据暂存起来。
-   **帧缓冲区（显存）**：显卡把渲染好的图像数据暂存于此，显示器以固定频率（如60Hz）从这里读取并刷新屏幕。这就是“垂直同步”技术管理的那个缓冲区。

###### 2. 操作系统内核：平衡IO性能

-   **页缓存（Page Cache）**：你读写文件时，数据先进入这块内存区域，操作系统异步地把它们刷入磁盘。这也是为什么突然断电有时会丢数据——因为数据还在缓冲区里没来得及存盘。
-   **Socket缓冲区**：发送网络数据时，数据先放到内核的“发送缓冲区”，网卡驱动慢慢取走；接收时同理。如果发送缓冲区满了，`write`/`send`函数就会阻塞。

###### 3. C/C++ 标准库：你最常碰到的“陷阱”

这是你在写C/C++代码时感触最深的：

-   **`printf` / `cout` 的缓冲区**：标准库为了减少系统调用，并不会每写一个字符就立刻显示到屏幕。它有三种策略：
    -   **全缓冲**：缓冲区满了才输出（默认写入普通文件时）。
    -   **行缓冲**：遇到换行符`\n`才输出（默认写入终端时）。
    -   **无缓冲**：立即输出（如`stderr`）。
-   **经典坑位**：程序崩溃时，你最后写的`printf("here")`没打印出来，就是因为没加`\n`或没调用`fflush(stdout)`强制刷新，数据还留在进程内存中，进程一崩就丢了。

###### 4. 应用层：主动设计的策略

-   **双缓冲（Double Buffering）**：常用于UI绘图或游戏渲染。你在屏幕上看到的是“前缓冲”，绘制操作在“后缓冲”上偷偷进行，画好后再交换指针。这能避免绘制过程中屏幕闪烁或撕裂。
-   **日志缓冲区（异步日志）**：高并发服务中，把日志先写入内存队列（缓冲区），由单独线程异步写盘。避免大量日志直接写磁盘拖垮主业务。

------

###### 💡 核心本质与误区澄清

**1. 缓冲区（Buffer） vs. 缓存（Cache）**
虽然中文常混用，但英文语境下区分明显：

-   **Buffer**：侧重**临时中转**，数据通常只经过一次，目的是**解耦流控**（如水管中间的水池）。
-   **Cache**：侧重**重复利用**，把计算过的结果存起来下次直接用，目的是**加速命中**（如Redis缓存）。

**2. 它们如何协同工作？**
以你在终端写一个`cout << "Hello" << endl`为例，数据流的旅程是：
应用层 `cout` 缓冲区（内存） → **刷新** → 操作系统内核的终端缓冲区（行缓冲） → **显示** → 硬件显存帧缓冲区 → 屏幕像素点。
每一层都在做中转和适配。

**🧠 如何不再混乱？**

**只需要记住一句话**：**缓冲区无处不在，它本质上是一个“蓄水池”**。
遇到任何一个“池子”，你只需要追问三个问题，就立刻清晰了：

1.  **这个池子连接了哪两端？**（例如：连接了CPU和硬盘）
2.  **为什么要设这个池子？**（因为速度不匹配，或者要批量处理）
3.  **水（数据）什么时候流出去（刷新）？**（满了？定时？手动强制？）

想清楚这三点，无论是编译器的内部缓冲、网络协议的滑动窗口，还是你代码里自定义的环形队列，都逃不出这个逻辑框架。如果你正在为特定场景（比如网络编程、音视频解码）的缓冲策略发愁，可以告诉我具体场景，我再给你细拆。😊

##### IO多路复用

###### 多线程

一个进程使用一个线程去处理，但是频繁的上下文切换有很大弊端

###### 上下文切换

一个线程在运行时，CPU 内部有大量**寄存器（Registers）**和**程序计数器（PC）**在记录它的运行现场（比如：当前执行到哪条指令、局部变量的中间值等）。

当发生切换时，操作系统（内核）必须做两件极其严谨的事情：

1.  **保存（Save）**：把当前线程的所有寄存器数据、程序计数器，小心翼翼地存到该线程的**内存控制块（TCB）**中，确保“断点”不会丢。
2.  **恢复（Restore）**：把下一个要执行的线程之前保存的数据，从内存中全部重新加载进 CPU 的寄存器里。

完成这两步，新的线程才能开始运行。

真正的性能杀手是 **CPU 缓存（Cache）和 TLB（地址转换缓存）失效**。

-   线程 A 运行时，CPU 的 L1/L2 高速缓存里装满了 A 喜欢用的数据，访问极快（纳秒级）。
-   切换到线程 B 后，A 的数据在缓存里就没用了。B 运行时需要去**主内存（RAM）**重新加载数据（慢几十上百倍）。
-   这种“数据冷启动”导致的延迟，远远大于保存几个寄存器的耗时。**线程切换得越频繁，缓存命中率越低，程序反而跑得越慢。**

什么时候出现上下文切换呢？

1.   主动让出，线程自己调用sleep,wait,或者是读取磁盘或者网络数据

2.   被动抢占，比如突然来了一个高优先级的线程

线程数量不是越多越好，如果线程数量大于CPU核心数，操作系统会在他们之间一直切换，花费大量时间去重新弄缓存，程序没有变快，而且还延迟变高了

###### 缓存

为什么有缓存？

CPU 的执行速度极快（纳秒级），而内存（RAM）的访问速度相对极慢（百纳秒级）。如果不做任何缓冲，CPU 大部分时间都在“干等”数据，算力被严重浪费。

**CPU 和内存之间交换数据的最小单元不是“字节”，而是“缓存行”**。

-   在现代 CPU（x86/ARM）中，**一个缓存行固定为 64 字节**。
-   当你读取一个 `int`（4字节）时，CPU 会把该地址**前后共 64 字节**的邻居数据全部一股脑加载进缓存。

###### select

```c++
#include <sys/select.h>

int select(int nfds, //最大文件描述符加一
           fd_set *readfds, //是否可读集合
           fd_set *writefds, //是否可写集合
           fd_set *exceptfds, //异常集合
           struct timeval *timeout);//超市时间
```

超时时间：NULL是永远等待 0是立即返回 

返回值：返回就绪的文件描述符总数，超时返回0,出错返回-1

fd_set本质上是一个位图（bitMap），不能直接赋值，要用宏

```c++
fd_set readset;          // 定义集合
FD_ZERO(&readset);       // 清空集合（把所有位设为0）
FD_SET(sockfd, &readset);// 把 sockfd 加入集合（把对应位设为1）
FD_CLR(sockfd, &readset);// 把 sockfd 从集合中移除
FD_ISSET(sockfd, &readset); // 判断 sockfd 是否在集合中（是否就绪），常用在 select 返回后
```

🔥注意：**select返回之后，他会修改传入的fd_set集合，只保留就绪的集合，因此，如果要循环调用select,必须每一次都重新FD_SET加入所有fd,不能使用之前的**

**缺陷**：

   1. 最大的fd数量限制一般是1024位，无法管理超过1024的连接

   2. 线性扫描效率低下：select返回后只能用`FD_ISSET` 从 `0` 到 `max_fd` **逐个遍历**判断哪个就绪。如果有 10000 个连接，只有 1 个活跃，你也要循环 10000 次，浪费 CPU。

   3. **内核态与用户态频繁拷贝**：每次调用 `select`，内核都要把 `fd_set` 从用户态拷贝到内核态。

      

      

      

      ```c++
      fd_set all_fds, read_fds;
      FD_ZERO(&all_fds);          // 1. 清空总名单
      FD_SET(listen_fd, &all_fds); // 2. 把监听socket放进总名单
      
      while (1) {
          // 【第一步】拷贝：总名单 -> 临时名单（为了保护总名单不被破坏）
          read_fds = all_fds;      
          
          // 【第二步】调用 select：内核检查临时名单，把没动静的擦掉
          select(max_fd + 1, &read_fds, NULL, NULL, NULL);
          
          // 【第三步】检查结果：遍历所有可能的 fd，看谁的位还是 1
          for (int i = 0; i <= max_fd; i++) {
              if (FD_ISSET(i, &read_fds)) {
                  // 说明 i 这个 fd 有数据来了！
                  // 如果是 listen_fd -> 执行 accept
                  // 如果是 client_fd -> 执行 recv
              }
          }
      }
      ```

      其实select就是内核把位图改变了，清空没有变化的，保留有变化的



#### DAY2



###### poll

select使用单一的fd_set位图，读写在一起，但是poll引入了一个结构体数组

```c++
struct pollfd {
    int fd;          // 你要监控的文件描述符（比如 socket）
    short events;    // 【输入】你关心什么事件（比如想读 POLLIN）
    short revents;   // 【输出】内核告诉你实际发生了什么事件
};
```

**最大的爽点**：内核只会修改 `revents`，绝不会碰 `events` 和 `fd`。所以你**不需要像 `select` 那样每次循环重新拷贝备份**！

| 常量        | 含义                                                    |
| :---------- | :------------------------------------------------------ |
| `POLLIN`    | 有数据可读（包括新连接）                                |
| `POLLOUT`   | 缓冲区有空位，可以写数据                                |
| `POLLERR`   | 发生错误（这是 `revents` 返回的，不能写在 `events` 里） |
| `POLLHUP`   | 对端挂断（`revents` 返回）                              |
| `POLLRDHUP` | 对端关闭写端（TCP 半关闭，有些系统需要额外定义）        |

函数原型

```c
#include <poll.h>

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
fds---结构体数组的首地址
nfds---数组里面有多少个元素
timeout---超时时间，单位是毫秒-1是死等，0是立即返回
```

返回值和select一样，返回就绪的fd数量，超时返回0,出错返回-1

```c++
#include <poll.h>

// 1. 定义数组（想要多大就多大，没有 1024 限制！）
struct pollfd fds[1024]; 

// 2. 初始化第一个元素（监听 socket）
fds[0].fd = listen_fd;
fds[0].events = POLLIN;   // 我只关心读事件
fds[0].revents = 0;       // 清空输出区

// 3. 初始化第二个元素（客户端 socket，假设已连接）
fds[1].fd = client_fd;
fds[1].events = POLLIN;
fds[1].revents = 0;

int nfds = 2; // 数组里实际有 2 个元素

while (1) {
    // 【核心调用】不需要拷贝备份！数组本身不会被破坏！
    int ready = poll(fds, nfds, -1); 
    //有ready个发生了事情
    if (ready <= 0) continue;

    // 【检查结果】遍历数组，看每个元素的 revents
    for (int i = 0; i < nfds; i++) {
        // 如果 revents 中包含 POLLIN，说明这个 fd 有数据可读
        if (fds[i].revents & POLLIN) {
            if (fds[i].fd == listen_fd) {
                // 有新连接，执行 accept，并把新 client_fd 加入数组（nfds++）
            } else {
                // 普通客户端发数据，执行 recv
                // 如果想写数据回去，修改 events 添加 POLLOUT
            }
        }
        // 检查是否出错或挂断
        if (fds[i].revents & (POLLERR | POLLHUP)) {
            // 关闭 fd，并把数组最后一个元素挪过来覆盖（或标记 -1 跳过）
        }
    }
}
```

所以说：fds是想要监控的所有socket列表，他必须包含监听socket+所有已经连接的客户端socket,当POLLIN时，监听socket进行accept,客户端socket需要recv

poll到底是干啥的？

函数调用相当于给操作系统内核说，这个是我想监视的socket列表，一共有nfds个，你帮我盯着，-1是我不设置超时，一直等，直到有任何一个socket有动静了，就告诉我

所以说select和poll其实都是让操作系统的内核去监视socket的变化。变化保存在revents,之后遍历所有socket，（这里做不到只遍历有变化的socket,而是遍历所有socket,通过检查标志，把有变化的挑出来）然后一个一个去处理有变化的

**流程**：

1.   内核监视（耗时
2.   返回统治，给了一个总数
3.   代码全遍历（耗时

所以说select和poll都是数据返回了，但是没有说是哪一个，要自己去翻遍全部socket寻找

| 对比维度           | **select**                                                   | **poll**                                                     |
| :----------------- | :----------------------------------------------------------- | :----------------------------------------------------------- |
| **你给内核的东西** | 3个“抽屉”（位图），用比特位代表 fd                           | 1个“数组”，每个元素是一个结构体                              |
| **输入和输出**     | **混在一起**。内核把抽屉改得乱七八糟，你每次调用前必须**重新拷贝**备份 | **彻底分开**。`events` 是你写的（输入），`revents` 是内核改的（输出），互不干扰 |
| **最大监视数量**   | 1024 个（硬限制）                                            | **没有硬限制**（改成 10000 个也行）                          |
| **代码写法**       | 用 `FD_SET`、`FD_ISSET` 折腾位                               | 直接操作结构体的 `.fd` 和 `.events`                          |

-   **`select`** 玩的是 **位（bit）**。
-   **`poll`** 玩的是 **结构体数组（struct pollfd）**。

###### epoll

epoll解决了两个问题：

1.   不用每次把很多fd从我的手里拷贝给内核
2.   不用返回后写for循环遍历所有fd（内核直接告诉你哪一个有变化，而不是几个有变化

-   **`select/poll` 的做法**：你每次都把一长串名单（所有 socket）递给内核，说：“帮我看一下。” 内核看完回来告诉你：“有 2 个人有动静，你自己去挨个问是谁吧。”

-   **`epoll` 的做法**：你直接在内核里**开了一个“托管室”**。你把 socket 名单**只交进去一次**，说：“这些归你管了。” 以后每次有动静，内核直接把你喊过来，**把有动静的 socket 一个一个塞到你手里**。

    

    ```c++
    int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    ```

    -   `EPOLL_CTL_ADD`：**添加**一个新 socket 进去监控。
    -   `EPOLL_CTL_MOD`：**修改**某个已存在的 socket 的监控事件（比如从"只读"改成"读写都看"）。
    -   `EPOLL_CTL_DEL`：**删除**某个 socket（比如客户端断开连接了）。

    参数 3：`fd`（你要操作的对象，给内核看的）

    -   **是什么**：就是那个**真正的、赤裸裸的 socket 文件描述符数字**（比如 `listen_fd = 3`）。
    -   **作用**：这是**系统调用传给内核的唯一凭证**。内核拿到这个数字 3，才知道要去内核的文件描述符表里找到"3 号"对应的那个 socket 对象，然后把它挂到红黑树上开始监控。
    -   **大白话**：你告诉前台："把 3 号客人（监听 socket）给我拉进房间。" 前台认的是**身份证号（数字 3）**，它必须看到这个数字，才知道去拉谁。

    参数 4：`event`（附带的操作说明/标签，给你自己看的）

    -   **是什么**：指向 `struct epoll_event` 结构体的指针（就是你之前填了 `ev.data.fd = listen_fd` 和 `ev.events = EPOLLIN` 的那个盒子）。
    -   **作用（拆分两层）**：
        1.  **告诉内核该怎么监视**（通过 `ev.events`，比如 `EPOLLIN`）。
        2.  **塞给内核一个"快递面单"**（通过 `ev.data`）。内核不关心你塞的是 `fd` 还是指针，它只负责**原封不动地保存这整块数据**，等 `epoll_wait` 返回时，**原样吐回给你**。

    

    

    

    `select/poll` 只有一个核心函数（`select` 或 `poll`），而 `epoll` 有三个，但分工明确：

    | 函数                 | 作用                                             | 大白话                                                       |
    | :------------------- | :----------------------------------------------- | :----------------------------------------------------------- |
    | **`epoll_create()`** | 在内核里创建一个“托管室”（返回一个 `epfd` 编号） | 去内核开个房间，拿个房卡（`epfd`）。                         |
    | **`epoll_ctl()`**    | 往托管室里**添加/删除/修改**要监控的 socket      | 拿着房卡，把客户端 socket 领进来，或者踢出去。               |
    | **`epoll_wait()`**   | 等着内核叫你                                     | 坐在门口等。内核出来说：“有 3 个人有动静”，并把**这 3 个人的具体信息**直接塞给你。 |

-   **`poll` 的结构体数组**：你有一个 `fds[10000]` 大数组，每次调 `poll` 都要把这个大数组拷进内核。

-   **`epoll` 的红黑树 + 就绪链表**：

    -   你用 `epoll_ctl` 添加 fd 时，内核把它放进一棵**红黑树**里（方便快速增删改查）。
    -   当某个 socket 有数据时，内核把它**单独拎出来**，放到一个**就绪链表（Ready List）** 里。
    -   调用 `epoll_wait` 时，内核**直接把就绪链表里的内容拷贝给你**。

>   **最重要的区别**：`epoll_wait` 返回给你的，是一个**只包含“有动静的 fd”的数组**，而不是全部 10000 个

```c++
//触发规则：当内核检查监视socket时，会拿实际状态匹配epollin，匹配到了才会触发
struct epoll_event {
    uint32_t events;   // 输入输出都有：你关心啥（EPOLLIN等），内核也告诉你发生了啥
    epoll_data_t data; // 【灵魂字段】这是一个共用体，你可以存 fd，或者存指针！
};
//回传数据模板：把这个data原样深拷贝一份，挂到红黑树节点里面，也就是epollwait返回的那一刻
//共用体
typedef union epoll_data {
    void    *ptr;      // 可以放指向自定义结构体的指针（高级用法）
    int      fd;       // 绑定fd,可以返回时知道fd
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;



#include <sys/epoll.h>

// 1. 开个托管室（在内核里）
int epfd = epoll_create(1); 

// 2. 把监听 socket 注册进去
struct epoll_event ev;
ev.events = EPOLLIN;    // 关心读事件
//【1】写面单：告诉内核，如果监听到这个 fd，记得把“3”写在返回的数据里
ev.data.fd = listen_fd; // 我要监听这个fd
// 【2】放包裹：告诉内核，把“3号监听socket”加入你的监控红黑树
epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

// 3. 主循环
while (1) {
    struct epoll_event ready_events[1024]; // 用来接内核返回的“有动静的 fd”
    
    // 【核心】阻塞等待，内核只把有动静的填到 ready_events 里
    int ready = epoll_wait(epfd, ready_events, 1024, -1);
    
    // 【爽点】不用遍历全部！只遍历内核返回来的这 ready 个！
    for (int i = 0; i < ready; i++) {
        int fd = ready_events[i].data.fd; // 直接拿到有动静的 fd
        
        if (fd == listen_fd) {
            // 新连接：accept，然后 epoll_ctl 加入托管室
        } else {
            // 普通客户端：直接 recv，因为确信它有数据
        }
    }
}
```

对于events

| 常用常量   | 含义（针对 Socket）                            | 使用场景                                    |
| :--------- | :--------------------------------------------- | :------------------------------------------ |
| `EPOLLIN`  | 可读事件（有数据来了，或者有新连接来了）       | **监听 socket** 和 **客户端 socket** 都用它 |
| `EPOLLOUT` | 可写事件（发送缓冲区有空位了）                 | 一般只在发大数据、被阻塞时才关心            |
| `EPOLLERR` | 发生错误（这是内核返给你看的，你**不能**输入） | 检查连接是否异常断开                        |
| `EPOLLHUP` | 对方挂断（内核返回的）                         | 检查连接是否被关闭                          |

监视：监视谁？监视什么操作？如果发生了，怎么告诉是谁？

所以说events=EPOLLIN解决了监视什么操作，内核会将第三各参数解析为内核指针作为红黑树的key插入树中，如果socket状态变化匹配EPOLLIN,那么内核才会把该节点移动到就绪链表

data.fd=listenfd，当epoll_wait要把就绪链表的数据拷贝到用户态时，内核直接把节点里面存的原样拷贝到用户态数据中

#### DAY3

##### epoll_wait

```c++
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
第二个是输出缓冲区数组，第三个是缓冲区容量，拷贝多少个到数组里面，第四各是超时控制
    -1永久阻塞 0立即返回 》0定时阻塞
```

**`epoll_wait` 是“从就绪链表（rdlist）中批量取出节点，并拷贝到用户态”的系统调用，如果没有节点，则让当前进程休眠。**

1.  **检查就绪链表（`rdlist`）是否为空**：
    -   内核查看 `struct eventpoll` 里的 `rdlist` 头指针。
    -   **如果不为空**：内核将链表中的节点数据（即你当初注册的 `epoll_event` 副本）批量拷贝到你传入的 `events` 数组中，然后返回拷贝的数量（`ready`）。
    -   **如果为空**：执行第二步。
2.  **将当前进程挂起（休眠）**：
    -   如果 `rdlist` 是空的，意味着当前没有任何 socket 有动静。
    -   内核将当前进程（你的线程）设置为睡眠状态（`TASK_INTERRUPTIBLE`），并将其挂到 `struct eventpoll` 的等待队列上。
    -   **进程在这行代码上阻塞**，CPU 去执行其他任务。
3.  **被唤醒后再次检查**：
    -   当某个 socket 触发事件，内核的回调函数会把节点放入 `rdlist`，然后唤醒等待队列上的进程。
    -   进程醒来后，`epoll_wait` 重新执行第一步，这次 `rdlist` 非空，于是取出数据返回。

**总流程**：

-   **`epoll_create1(0)`**：内核创建 `struct eventpoll`（内含**空的红黑树** + **空就绪链表**）。
-   **`epoll_ctl(ADD, listen_fd, &ev)`**：
    1.  内核把 `listen_fd` 对应的 `struct file` 作为 **Key**，插入**红黑树（`rbr`）**。
    2.  同时，内核把 `&ev` 拷贝一份作为 **Value**，挂在树的节点上。**注意：这一步不会碰就绪链表**。
-   **（过了一会）客户端发来 SYN，三次握手完成**：
    1.  网卡中断，内核把新连接放入 `listen_fd` 的 accept 队列。
    2.  内核检查 `listen_fd` 的状态，发现匹配了之前注册的 `EPOLLIN`。
    3.  内核执行回调函数：将红黑树中 `listen_fd` 对应的那个节点，**放入 `rdlist`（就绪链表）的尾部**。
    4.  分配数组ready_events
-   **你的程序调用 `int n = epoll_wait(epfd, ready_events, 64, -1)`**：
    1.  内核发现 `rdlist` 非空，从链表头部**彻底**取下一个节点。
    2.  内核遍历 `rdlist`，把节点中存储的 `data.fd` 和 `events` 拷贝到你的**用户态数组** `ready_events` 中,返回拷贝数量n,**如果是空的，进程在此处休眠**。
    3.  LT模式下：拷贝完了之后，内核检查socket实际状态，如果是客户端socket的情况会检查其缓冲区环有没有未读数据，**如果有**，那么内核把这个节点重新放回**rdlist尾部**，**如果没有**，那就不进行操作，这个节点彻底消失
    4.  ET模式下，无论socket是否有剩余数据，绝对不会自动重新入队。**这就是 ET 模式要求你必须用 `while` 循环把 `recv` 读到 `EAGAIN` 的原因**——因为内核只通知你一次，你不读干净，剩下的人就永远丢在缓冲区里没人管了。
    5.  循环遍历数组并且处理：如果是监听fd,那么就accept并且把这个新的客户端加入到红黑树里面，如果是客户端，那么就发送接受数据

**`epoll_wait`** 是唯一一个**不往红黑树写数据**的函数，它**只读就绪链表**。如果链表空，它让你睡觉；链表非空，它把数据捧给你。

LT保证安全，ET适用与高并发，大流量服务器

##### send

当你的客户端程序调用 `send()` 时，数据流向是这样的：

1.  **客户端（你的程序）** 构造数据，调用 `send()`。
2.  **操作系统内核（协议栈）** 拿到数据，封装成 TCP/IP 报文，放进网卡的**发送队列（内存缓冲区）**。
3.  **网卡驱动** 从队列取出数据，交给 **网卡硬件**。
4.  **网卡硬件** 把数据转成电信号，通过网线发出去。

**结论**：

-   **客户端**是那个**“说话的人”**（负责组织语言）。
-   **内核协议栈**是那个**“邮局分拣员”**（负责打包、写地址）。
-   **网卡**只是那个**“信使脚下的自行车”**（负责物理运输）。



#### DAY4

```c++
if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
```

-   **执行过程**：`ev`（当前触发的事件）与这个“错误掩码”进行按位与。
    -   如果 `ev` 包含其中**任意一种**错误位，结果就**非 0**（条件为真），立即关闭客户端。
    -   如果 `ev` 一个错误位都不包含，结果就是 **0**（条件为假），安全跳过。

#### DAY5

##### 为什么客户端要用poll？

话说可以理解服务端使用epoll用来处理多个客户端的并发，但是客户端只连接一个服务端，为什么还要用poll？

因为客户端用poll是用来处理两个输入源（键盘和网络）并发

如果不使用poll,那么客户端只能实现 要么先收后发，要么先发后收，只能干一件事，另一件事要等待，比如说是先收后发：如果接受不到消息，就一直堵塞，发不了消息

但是使用了poll之后，就可以实现边接受键盘，边接受网络

**那么为什么不用epoll而是poll？**

因为客户端只需要监听两个文件描述符，一个键盘一个网络连接，而epoll属于大炮打蚊子，epoll是为了海量连接准备的，创建开销更大，但是poll在fd少的时候，和epoll几乎没有区别，更标准更加轻量化

#### DAY6

##### 共用体是啥？里面的指针是啥



```c
typedef union epoll_data {
    void    *ptr;      // 可以放指向自定义结构体的指针（高级用法）
    int      fd;       // 【最常用】存 socket 的文件描述符
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;
```

这个是一个共用体，在epoll_wait收到的就绪链表event里面，每一个就绪的元素都有一个这个共用体：目的是为了节省空间内存，让效率更加快速

**对于指针？**

它是一个**“万能指针”**（`void*` 表示它可以指向任何类型的数据）。

-   它的大小固定（8 字节，在 64 位系统上），和 `data.fd` 共用同一块内存。
-   它存在的目的：**让你能把“文件描述符（fd）”和“跟这个 fd 相关的所有业务数据（缓冲区、回调函数、状态码）”捆绑在一起。**
-   如果不用 `ptr`，当服务器收到消息时，它只知道 `fd=5`，必须去问“fd=5 是谁？”，然后找到这个人的昵称和消息队列。
    如果用 `ptr`，当服务器收到消息时，它直接拿到 `Client*`，里面**已经存好了**昵称 `"张三"` 和待发送的缓冲队列，直接把消息塞进去就行。



##### 编译运行

```c
cmake -S . -B build
cmake --build build
```

命令一：`cmake -S . -B build`

-   **`-S .`**（Source）：指定**源代码目录**为当前目录（`.`）。CMake 会去这里找你的 `CMakeLists.txt`。
-   **`-B build`**（Build）：指定**构建输出目录**为 `build` 文件夹。如果该文件夹不存在，CMake 会自动创建。
-   **内核动作**：
    1.  读取当前目录的 `CMakeLists.txt`。
    2.  检测你的编译器（GCC/Clang/MSVC）、系统类型（Linux/Windows）。
    3.  根据你的 `CMakeLists.txt` 中的 `add_executable(chat_server server.cpp)` 等指令，在 `build` 目录下生成**平台原生的构建文件**（Linux/Mac 下是 `Makefile`，Windows 下是 `.sln`/`.vcxproj` 或 `ninja.build`）。

**此时，`build` 文件夹里全是中间文件（CMakeCache.txt、Makefile 等），还没有任何 `.o` 目标文件或可执行文件。**

命令二：`cmake --build build`

**这是“编译（Build）+ 链接（Link）”阶段。**

-   **`--build`**：这是一个通用指令，告诉 CMake 去调用**底层构建工具**（比如 Linux 下的 `make`，Windows 下的 `msbuild` 或 `ninja`）。
-   **`build`**：告诉 CMake 去哪个目录执行构建。
-   **内核动作**：
    1.  CMake 进入 `build` 目录，找到上一步生成的 `Makefile`（或 `.sln`）。
    2.  执行构建命令（等价于你在 `build` 目录里手动敲 `make`）。
    3.  编译器开始编译 `server.cpp` 和 `client.cpp`，生成 `.o` 目标文件，最后链接生成最终的**可执行文件** `chat_server` 和 `chat_client`（它们会出现在 `build/` 目录下）。



##### EPOLLOUT

**🔥 如果你“一直监听 EPOLLOUT”会发生什么？（CPU空转）**

假设你的代码注册事件时写成了：`EPOLLIN | EPOLLOUT`（永远监听可读和可写）。

**场景推演（水平触发 LT，epoll 的默认模式）：**

1.  连接建立成功，内核为这个 socket 分配了 **8KB 的发送缓冲区**，此时缓冲区是 **空的**（空闲空间 = 8KB）。
2.  因为缓冲区是空的，满足 `EPOLLOUT`（有空间）的条件。
3.  `epoll_wait` 返回，告诉你这个 fd 可写。你此时可能**没有任何数据要发**（`out_buffer` 是空的），但系统还是通知了你。
4.  你无事可做，继续调用 `epoll_wait`。
5.  因为缓冲区**依然是空的**（依然满足“有空间”条件），`epoll_wait` **立刻又返回了**。
6.  **死循环开始**：`epoll_wait` 一秒钟可能返回几万次，但你没有数据要发，CPU 被这个空转吃满，而业务数据（收消息）却得不到处理。

**这就是文中所说的：“疯狂返回这个 fd 的可写事件，导致 CPU 空转（忙轮询）”。**

----



**✅ 为什么要“动态加/减”？（正确的做法）**

**核心原则**：**只有当我手里确实有苹果（`out_buffer` 里有数据）时，我才举手（注册 `EPOLLOUT`）告诉老师（内核）我要发言。数据发完了，我就把手放下（取消 `EPOLLOUT`）。**

**流程演示：**

-   **平时（空闲状态）**：注册的事件只有 `EPOLLIN`（只监听读）。哪怕发送缓冲区能塞进一座泰山，`epoll_wait` 也**不会**因为“能写”而唤醒你，CPU 非常安静。
-   **触发发送（有数据要发）**：
    1.  你的业务逻辑调用 `send()`，发现内核缓冲区满了，只发出去了 100 字节，剩下 900 字节还在 `out_buffer` 里。
    2.  **关键动作**：你立刻调用 `update_epoll_events`，把 `EPOLLOUT` **加上**。
    3.  现在事件变成 `EPOLLIN | EPOLLOUT`。
    4.  因为发送缓冲区有了空闲，`epoll_wait` 立即返回可写事件，你调用 `handle_client_write` 去把 `out_buffer` 里剩余的 900 字节拼命塞给内核。
    5.  直到所有数据塞完，`out_buffer` 变为**空**。
    6.  **再次关键动作**：你再次调用 `update_epoll_events`，把 `EPOLLOUT` **去掉**。
    7.  事件回到 `EPOLLIN` 状态，世界再次安静下来，不再有无效的“可写”通知。





##### queue_message

```
. 为什么“不能”在 queue_message 里直接 send？
假设你在 queue_message 里直接写：

cpp
send(client_fd, message.c_str(), message.size(), 0);
在非阻塞模式下，send 会有两种结局：

结局一（好运气）：内核的发送缓冲区有空位，数据成功发出去了。一切正常。

结局二（坏运气）：对方电脑死机、网线松了、或者对方应用层处理太慢，导致 TCP 窗口关闭。此时内核发送缓冲区满了。send 不会等待，而是立刻返回 -1，并设置 errno = EAGAIN 或 EWOULDBLOCK。

请问，当发生结局二时，你的代码该怎么处理？
你只能：

把没发完的数据先存起来（存到 out_buffer）。

等着 EPOLLOUT 事件。

等内核通知你“有空间了”，你再把存起来的数据发出去。

结论：只要你想写出健壮的网络程序，out_buffer 和 EPOLLOUT 是绕不开的。因为无论你多着急，网络带宽和对端的接收能力不由你的代码控制。

2. 为什么策略是“不立即调用 send”？（纯粹的异步解耦）
既然无论如何都要处理 EAGAIN，那就干脆把逻辑全部统一放到事件循环里处理。

设计哲学：业务层（queue_message）只负责“生产消息”，网络层（handle_client_write）只负责“发送消息”。

在 queue_message 里不碰 send，意味着业务逻辑永远不需要关心 EAGAIN 是什么东西。它只管把字符串往 out_buffer 后面一丢，然后拍拍屁股走人。

这样做最大的好处是：避免了你陷入“部分发送”的泥潭。如果你在 queue_message 里调用 send，发送了 50 字节，还剩 50 字节，你就必须维护“下次从哪个位置继续发”的偏移量，代码会瞬间变得极其丑陋。

3. 性能会不会变差？（延迟担心）
你可能会想：“放到队列里，得等到下一轮 epoll_wait 触发才能发，这不是白白浪费了几十微秒吗？”

实际上，现代高性能网络库（如 Netty、muduo、Redis）的通用做法是“先尝试立即发送，失败再入队”。虽然你给的描述是“策略：不是立即调用 send”，但在真正的工业级代码中，通常会这样优化 queue_message：

cpp
void queue_message(int epoll_fd, int client_fd, const string& msg) {
    // 1. 先尝试直接发送（减少一次 epoll 循环延迟）
    ssize_t n = send(client_fd, msg.c_str(), msg.size(), 0);
    if (n == (ssize_t)msg.size()) {
        return; // 运气好，一次性发完，皆大欢喜
    }

    // 2. 如果没发完或者遇到 EAGAIN
    if (n > 0) out_buffer.append(msg.substr(n)); // 发了部分，剩下的入队
    else out_buffer.append(msg);                 // 完全没发出去，全部入队

    // 3. 开启 EPOLLOUT，等待下一次可写
    update_epoll_events(epoll_fd, client_fd); 
}
但是，如果你的描述严格限定为“不立即 send，全部入队”，那这种设计通常是出于 “单线程Reactor的极简主义” —— 为了确保所有的 I/O 操作（读和写）绝对只在主循环的 handle_client_read/write 中发生，避免在多线程环境下抢锁导致的复杂性。这种设计下，延迟增加的是 epoll_wait 一轮循环的时间（通常不到 0.1ms），对于非实时性业务完全可以接受。

💡 一句话总结你的代码逻辑
queue_message 就是给 TCP 内核缓冲区加了一个“应用层泄洪池”。

如果直接发（内核有空位）：水直接流走。

如果内核堵了（EAGAIN）：水先存在你家的池子（out_buffer）里。

开启 EPOLLOUT：相当于在池子底部装了一个传感器，当
```

##### 不同操作系统的换行格式不同

Linux:   \n
Windows: \r\n

所以在读取命令的时候要格外注意转换

#### DAY7

##### EPOLLRDHUP

. 它和 `EPOLLHUP` 有什么区别？

-   **`EPOLLHUP`（本地挂断）**：代表**本端**的套接字发生了错误或挂断（比如管道断裂、设备错误）。这通常是一个**意外**或异常状态。
-   **`EPOLLRDHUP`（远程挂断）**：代表**对端**正常地关闭了连接（优雅地挂了电话）。这是一个**预期内**的事件。

2. **在你的代码中，它是怎么起作用的？**

你的两段代码都出现了这个组合：



```
// 注册事件时
event.events = EPOLLIN | EPOLLRDHUP;

// 主循环判断时
if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
    close_client(epoll_fd, fd);
    continue;
}
```



**逻辑解读**：

1.  **注册时带上它**：告诉内核，不仅要关心“有数据可读”（`EPOLLIN`），还要关心“对方是否关闭了”（`EPOLLRDHUP`）。
2.  **触发时**：当对方关闭连接（发送 FIN 包）时，`epoll_wait` 会返回这个事件，然后你的代码立刻调用 `close_client` 清理资源。

**3. 为什么不只靠 `recv` 返回 0 来判断关闭？**

你可能在想：“我直接用 `recv` 读到 0 字节，不就代表对方关闭了吗？为什么还要单独监听这个事件？”

-   **效率更高**：`EPOLLRDHUP` 可以让你**提前感知**关闭事件，而不必等到下次调用 `recv`。
-   **区分“关闭”和“无数据”**：如果没有 `EPOLLRDHUP`，当 `epoll_wait` 被唤醒时，你无法仅凭事件标志区分“对方关闭了”和“单纯没数据可读”。带上它，你可以在 `recv` 之前就知道对方已经挂断了。

##### 关闭模式

有两种关闭模式

1.   是使用ctrl+c或者网路断线，调用close的时候---直接丢弃数据
2.   是用户使用QUIT命令的时候，需要等待数据发完之后再断开---优雅关闭

##### 一段代码

```c
void update_epoll_events(int epoll_fd, int client_fd) {
    const auto it = clients.find(client_fd);
    if (it == clients.end()) {
        return;
    }

    epoll_event event{};
    event.data.fd = client_fd;
    event.events = EPOLLRDHUP;

    if (!it->second.close_after_write) {
        event.events |= EPOLLIN;
    }

    if (!it->second.out_buffer.empty()) {
        event.events |= EPOLLOUT;
    }

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) == -1) {
        std::cerr << "epoll_ctl MOD failed, fd=" << client_fd
                  << ", error=" << std::strerror(errno) << '\n';
    }
}
```



-   **查找客户端上下文**

    cpp

    ```
    const auto it = clients.find(client_fd);
    if (it == clients.end()) return;
    ```

    

    从全局（或类成员）的 `clients` 容器（通常是 `std::unordered_map`）中查找该文件描述符对应的上下文数据。如果找不到，说明连接已失效，直接返回。

-   **初始化事件结构体**

    cpp

    ```
    epoll_event event{};
    event.data.fd = client_fd;
    ```

    

    将 `event` 清零，并绑定对应的文件描述符，以便 epoll_wait 返回时能知道是哪个连接触发了事件。

-   **核心：动态计算监听事件（按位或操作）**

    -   **基础事件：`EPOLLRDHUP`**
        这是 Linux 提供的专门检测对端关闭连接的事件（TCP 收到 FIN 包）。加上它，可以第一时间感知客户端主动断开，而不必依赖读写返回 0。

    -   **条件一：监听读事件（`EPOLLIN`）**

        cpp

        ```
        if (!it->second.close_after_write) {
            event.events |= EPOLLIN;
        }
        ```

        

        只有当 `close_after_write` 标志为 `false` 时，才监听读事件。
        *设计意图*：如果该连接即将在发完数据后关闭（半关闭状态），此时没必要再监听读事件，避免频繁触发无用的事件循环，也防止读到的数据无处处理。

    -   **条件二：监听写事件（`EPOLLOUT`）**

        cpp

        ```
        if (!it->second.out_buffer.empty()) {
            event.events |= EPOLLOUT;
        }
        ```

        

        只有当应用层的输出缓冲区（`out_buffer`）**有数据待发送**时，才监听写事件。
        *设计意图*：这是 **ET（边缘触发）模式下的标准写法**。应用层无法一次性发完所有数据时，才注册 `EPOLLOUT`，让内核在发送缓冲区有空位时通知程序继续发送。数据发完后，务必通过此函数移除 `EPOLLOUT`，避免 epoll 一直疯狂触发写就绪事件（即“忙轮询”），消耗 CPU。

-   **执行修改并处理异常**

    cpp

    ```
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) == -1) {
        // 打印错误
    }
    ```

    

    调用 `epoll_ctl` 使用 `EPOLL_CTL_MOD` 选项修改该 fd 的监听集（假设该 fd 早已通过 `EPOLL_CTL_ADD` 添加过）。如果失败，打印错误日志（通常是因为 fd 已失效或 epoll 实例被关闭）。

##### 关于哈希表

```c
auto it = clients.find(client_fd); // 假设找到了 fd=7 的那一行
const Client& client = it->second; // 这里取出的就是 fd=7 对应的那个 Client 对象
```

##### 关于&

**为什么不用&&而用&？**

因为&是把两项做二进制运算，但是&&只看真值，有一些情况是不符合的

对于选项宏一定要用&

#### DAY8

7.22

##### \#pragma once

**“这个头文件（.h），在编译整个项目时，只允许被打开和读取一次，谁敢再包含（#include）我，我就自动忽略谁。**

*用 `#pragma once` 的好处**：

-   **省心**：不用起名字，不用写三行，一行搞定。
-   **快**：编译器不需要像处理 `#ifndef` 那样打开文件去比对宏名，而是直接通过文件系统判断，编译速度会略微提升（大型项目很明显）。
-   **不出错**：绝对不会因为宏名重复而误伤头文件。



##### 为什么命名空间的名字一样不房子一起？

首先，使用命名空间是为了放置函数重名

其次，用文件隔开是为了放置逻辑混乱，让代码结构更加清晰

##### 为什么要封装服务器类

1.   舍去了复杂的传参数环节，直接在类里面直接调用成员变量，不用当作参数传进去了
2.   解决资源释放的难题RALL,直接封装一个析构函数不用每次都close
3.   隐藏内部细节，比如说main里面太复杂，后面直接整合称一个初始化函数调用就好了



#### DAY9

##### 什么时候用map什么时候用set？

map---保存键值对---给定一个键可以查到他的值

set---保存键---只能看这个键是否存在于列表中

**什么时候用哪一个呢？**

如果需要用A去查B---使用map

如果只需要知道A是否存在---用set

set比map省内存，但是查找速度差不多，但是set在CPU缓存上更友好

##### 业务逻辑验证步骤

1.   验证登陆状态
1.   解析命令参数





#### DAY10

##### accept4

这是LINUX系统调用：**在接受新连接的同时，直接为新创建的 socket 文件描述符设置标志（flags），一步到位，避免额外的系统调用。**

```c
accept4(listen_fd_,
        //服务端监听socket
        reinterpret_cast<sockaddr*>(&client_address),
        //输出参数：输出的客户端IP、端口保存在这里
        &client_address_length,
        //输入输出参数，传入地址结构体的大小，内核返回实际大小
        SOCK_NONBLOCK | SOCK_CLOEXEC);
        //设置为非阻塞模式，自动执行时关闭
```

##### LT&ET

```c
while(true){
    int ready=epoll_wait(epoll_fd,events,MAX_EVENTS,-1);
    for(int i=0;i<ready;i++){
        int fd=events[i].data.fd;
        uint32_t ev=events[i].events;
        if(fd==listen_fd)handle_accept();
        else{
            if(ev&EPOLLIN)handle_read(fd);
            if(ev&EPOLLOUT)handle_write(fd);
            if(ev&EPOLLERR|EPOLLHUP)close_client(fd);
        }
    }
}
```

这是epoll的两种工作模式

假设现在客户端发来10KB数据,并且第一次只读了3KB数据：

​       **LT**:会一直返回，知道数据读取完毕

​       **ET**:只通知一次，没有读完也不会再通知了

###### accpect

eoll默认就是LT模式

```c
void handle_accept() {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    
    // LT 允许：只尝试一次
    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &len);
    if (client_fd > 0) {
        set_non_blocking(client_fd);
        add_to_epoll(client_fd, EPOLLIN); // 注册读事件
    }
    // 如果 client_fd == -1 且 errno == EAGAIN，没关系，下次 epoll 还会叫醒我。
}
```

ET

```c
void handle_accept() {
    while (true) { // 必须死循环！
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        
        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &len);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 所有连接都处理完了，安全退出
            }
            // 真正的错误，退出
            break;
        }
        set_non_blocking(client_fd);
        add_to_epoll(client_fd, EPOLLIN | EPOLLET); // 注意要加 EPOLLET
    }
}
```

LT

```c
void handle_read(int fd) {
    char buf[1024];
    // LT 允许：只尝试读一次
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    
    if (n > 0) {
        append_to_buffer(fd, buf, n);
        parse_command(fd); // 尝试解析
    } else if (n == 0) {
        close_client(fd); // 客户端关闭
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            close_client(fd); // 真正的错误
        }
    }
    // 如果有粘包（一次性收了 3 条命令），因为只读了一次，可能只处理了 1 条。
    // 不要紧！由于缓冲区还有数据，epoll_wait 会马上再次触发 handle_read。
}
```

ET

```c
void handle_read(int fd) {
    char buf[1024];
    while (true) { // 必须死循环！
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        
        if (n > 0) {
            append_to_buffer(fd, buf, n);
            // 注意：ET 模式下，你要小心处理“半条命令”。
            // 如果读到一半，粘包还没拼完，可以继续循环读数据。
        } else if (n == 0) {
            close_client(fd);
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 读完了！内核缓冲区空了，安全退出循环
                break;
            } else {
                close_client(fd); // 报错退出
                break;
            }
        }
    }
    // 循环结束后，再去解析 in_buffer 里的完整命令
    parse_all_commands(fd); 
}
```

LT&ET

```c
// 主动发消息：把数据入队，并注册 EPOLLOUT
void queue_message(int fd, const string& msg) {
    clients_[fd].out_buffer += msg;
    // 关键：动态修改 epoll 事件，开始监听写
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT; 
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

// handle_write（无论是 LT 还是 ET，建议都用“死循环发送”的 ET 写法，性能最好）
void handle_write(int fd) {
    Client& client = clients_[fd];
    while (true) { // 循环发，直到发完或缓冲区满
        ssize_t n = send(fd, client.out_buffer.data(), client.out_buffer.size(), MSG_NOSIGNAL);
        
        if (n > 0) {
            client.out_buffer.erase(0, n);
            if (client.out_buffer.empty()) {
                break; // 发完了，跳出循环
            }
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 对方接收窗口满了，暂时发不动，等下次 EPOLLOUT
            } else {
                close_client(fd); // 发送失败，断开
                return;
            }
        }
    }
    
    // 如果数据全部发完，立刻注销 EPOLLOUT，防止下次 epoll 空转（CPU 飙升）
    if (client.out_buffer.empty()) {
        epoll_event ev;
        ev.events = EPOLLIN; // 只保留读事件
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    }
}
```





##### ssize_t

**这是一个专门用来接收系统调用（read/write）返回值的类型，它的正数表示长度，负数（特别是-1）表示错误。*

##### 为什么服务端使用mysql但是客户端使用sqlite呢？

| 特性         | SQLite                         | MySQL / PostgreSQL                 |
| :----------- | :----------------------------- | :--------------------------------- |
| **架构**     | **嵌入式**，无服务器进程       | **客户端-服务器**（Client-Server） |
| **部署**     | **零配置**，无需安装           | 需要安装、配置和运维               |
| **数据库**   | **单个普通磁盘文件**           | 通常是一系列文件或目录             |
| **并发**     | **低**，适合单用户或少量写入   | **高**，支持多用户高并发           |
| **功能**     | 基础，轻量                     | 丰富，支持存储过程、高级索引等     |
| **安全**     | 依赖文件系统权限               | 内置用户、角色和细粒度权限控制     |
| **适用场景** | 移动应用、桌面软件、嵌入式设备 | Web 应用、企业级系统、数据分析     |

**如果把服务端的 MySQL 换成 SQLite（灾难级）**

-   **后果**：服务端**直接宕机**。
-   **原因**：SQLite 是**“文件级锁”**，写入操作是**串行化**的（一次只能一个人写）。当几百个用户同时发消息时，SQLite 会直接报错 `database is locked`（数据库被锁）。
-   **结论**：**MySQL 是“多人协作的共享文档”**，专门为高并发读写设计；**SQLite 是“单人单机的 Word 文档”**，只适合一个人操作。服务端不用 SQLite 是**铁律**。





**如果把客户端的 SQLite 换成 MySQL（魔幻级）**

-   **后果**：App 安装包变得巨大（MySQL 安装包几百 MB），而且手机根本跑不动。

-   **原因**：MySQL 是一个**独立的服务进程**（需要后台一直运行），占用内存至少几百 MB，手机电量分分钟耗光。而且客户端网络不稳定时，连接 MySQL 会频繁超时。

-   **结论**：**客户端用 MySQL 就像让手机天天背着个发电机跑**，完全不合理。

    





**最重要的安全问题（客户端绝对不能连 MySQL）**

如果客户端直接连服务端的 MySQL：

-   你需要把 MySQL 的 **IP 地址、端口、账号密码** 硬编码写在 App 代码里。
-   黑客只要**反编译 App**，就能拿到这些信息，直接远程登录你的数据库，**拖走所有用户的聊天记录和密码**。
-   **这是毁灭性的安全漏洞**，任何正规公司都不会这么做。



#### DAY11

##### 内核？

内核就是一个操纵硬件层的管家，因为应用层不能直接和硬件层联系，所以需要内核在中间调度

所有程序要用硬件必须经过内核，**跑在内存里的核心代码（调度器、驱动、协议栈）**

##### 内核态

是cpu的一种运行模式，此时，它可以执行任何特权的指令，全线对应**RIng0**（最高权限）

##### 用户态

CPU的另一种运行模式，此时，CPU被严格限制，只能执行加减乘除之类，访问自己的内存空间，不能触碰硬件和外设内存，权限级别是Ring3（最低权限）

##### CPU模式切换

一般模式下，执行一些简单的代码的时候line.pop_back()`、`username.size()`、`if (command.name == "HELP")——算术逻辑的时候，CPU在用户态跑，极快，不涉及硬件

**系统调用时，执行recv**，CPU主动从用户态切换到内核态，此时，CPU开始运行内核的代码，因为拥有最高权限，所以内核直接读取网卡DMA内存，把数据拷贝到recv给的那个buffer里面，拷贝完成后，CPU切换回用户态，运行我的c++代码

**硬件中断的时候**：CPU正在用户态跑while循环，此时网卡突然来了一个数据包，打断了CPU,CPU不管之前用户态跑什么代码，强行保存现场，**切换内核态**，跑网卡驱动，把数据收进内核缓冲区，**收完后切换回用户态**，epllwait会返回告诉有数据



##### 什么是心跳检测？

###### 为什么？

在TCP四次挥手中，当客户端发送FIN包，服务端受到FIN包的时候，表示正常关闭了

但是很多时候，比如说网卡被拔，客户端断电————客户端换没有来得及发欧式能够fin包就中断了，此时服务端并不知道

###### 怎么做？

最简单的模型是 **Ping-Pong**（客户端/服务器互相试探）：

1.  **服务端发起**（常用）：服务端开启一个定时器（比如每 30 秒）。遍历所有已登录的客户端，发送一个特殊的命令或一个字节（比如 `PING`）。
2.  **客户端响应**：客户端收到 `PING` 后，必须立即回复一个 `PONG`。
3.  **判断超时**：服务端为每个客户端记录一个 `last_pong_time`。如果服务端连续发了几次 `PING`，比如过了 60 秒还没收到 `PONG`，服务端就认定连接已死，直接调用 `close_client(client_fd)` 清理掉。

>   **双向模式**：客户端也可以主动发心跳，告诉服务端“我还活着”。服务端只需检查在固定时间窗口内有没有收到任何数据（任何数据都能当作心跳）。



##### enum

###### enum class



是 **C++11 引入的强类型枚举**

```c
enum Color { RED, GREEN, BLUE };

Color c = RED;        // ✅ 可以
int x = RED;          // ✅ 隐式转为 int，RED = 0
c = 1;                // ✅ 可以赋值整数，危险！
if (c == 1) { ... }   // ✅ 可以和整数比较
```



```c
enum class Color { RED, GREEN, BLUE };

Color c = Color::RED;     // ✅ 必须加作用域
// int x = c;             // ❌ 禁止隐式转 int
// c = 1;                 // ❌ 禁止直接赋整数
// if (c == 1) { ... }   // ❌ 禁止和整数比较
if (c == Color::RED) { ... }  // ✅ 只能和同类型比较
```

为什么需要enumclass？

问题 1：命名冲突

cpp

```cpp
enum Color { RED, GREEN, BLUE };
enum Status { OK, ERROR, RED };   // ❌ 编译错误！RED 重复定义了

// 用 enum class 解决：
enum class Color { RED, GREEN, BLUE };
enum class Status { OK, ERROR, RED };  // ✅ 不冲突，作用域隔离
```

问题 2：隐式转换的坑

cpp

```cpp
enum Color { RED, GREEN, BLUE };
void setColor(Color c) { ... }

setColor(1);           // ❌ 编译通过但逻辑错误！1 不是有效颜色
setColor(100);         // ❌ 同样编译通过

// enum class：
enum class Color { RED, GREEN, BLUE };
setColor(1);           // ❌ 编译错误！类型不匹配
setColor(Color::RED);  // ✅ 必须显式指定
```

问题 3：不同枚举意外相等

cpp

```cpp
enum Color { RED = 0, GREEN = 1 };
enum Status { OK = 0, ERROR = 1 };

Color c = RED;
Status s = OK;
if (c == s) { ... }    // ❌ 传统 enum 允许！逻辑荒谬但编译通过

// enum class：
if (c == s) { ... }    // ❌ 编译错误！类型不匹配
```



**`enum` 是"披着枚举外衣的整数"，弱类型、易出错；`enum class` 是真正的独立类型，强类型、更安全。**

##### 结构体和类有什么区别？

```c


class Person {
    int age;          // 默认 private，外部不可访问
    void sayHi();     // 默认 private
};

struct Person {
    int age;          // 默认 public，外部可直接访问
    void sayHi();     // 默认 public
};


```

使用惯例

| 特性         | **struct（结构体）**                             | **class（类）**                                           |
| :----------- | :----------------------------------------------- | :-------------------------------------------------------- |
| **核心定位** | **“数据的容器”**（纯数据聚合）                   | **“完整的对象”**（数据 + 行为 + 封装）                    |
| **成员权限** | 几乎全部是 **`public`**，直接暴露数据            | 数据通常是 **`private`**，通过公开方法（`public`）访问    |
| **复杂逻辑** | 最多带几个简单的构造函数，**极少**带复杂业务函数 | 包含核心业务逻辑（`SendMessage()`, `Connect()` 等）       |
| **不变量**   | 不维护任何“状态合法性”检查（赋值什么就是什么）   | 维护内部状态一致性（比如 `age` 不能为负数，由类方法保证） |



##### explicit



**加了 `explicit` 的构造函数，必须由你主动、显式地调用，编译器不能偷偷摸摸帮你把别的类型变成这个类**

##### 构造函数是什么？？

构造函数是 C++ 中**专门用来“初始化对象”的特殊成员函数**

##### private与public

| 访问权限              | 应该放什么？                                                 | 具体例子（聊天系统）                                         |
| :-------------------- | :----------------------------------------------------------- | :----------------------------------------------------------- |
| **`private`（私有）** | **① 所有的成员变量（数据）** **② 内部辅助函数**（外部不需要知道的底层脏活累活） **③ 维护正确性的内部状态** | `socket_fd_`（连接句柄） `token_`（登录凭证） `last_heartbeat_`（心跳时间） `ParseRawData()`（解析二进制流的内部函数） |
| **`public`（公有）**  | **① 构造函数/析构函数**（创建和销毁对象） **② 对外业务接口**（外部想用这个类能调用的所有操作） **③ 必要且安全的 Getter/Setter** | `UserSession(int fd)`（构造函数） `SendMessage()`（发消息） `Disconnect()`（断开连接） `GetUserId()`（只读查询） |

##### 存公共消息为什么使用队列？

**因为聊天消息是有“先后顺序”的，必须“先来先发”。** 

为什么不用其他数据结构（链表、数组、双端队列）？

-   **链表（`std::list`）**：也可以做队列（`push_back` + `pop_front`），但每个节点要额外存两个指针（8~16 字节），内存开销大，且缓存不友好。在长连接高并发下，频繁 `new` 节点会加重内存碎片。
-   **数组/向量（`std::vector`）**：`push_back` 很快，但头部删除（`erase(begin)`）是 O(n) 操作，数据量一大性能暴跌。你的 `std::string` 就是这种结构，其实不太高效，但代码简单，适合学习。
-   **环形缓冲区（Ring Buffer）**：这是工业级（如 Nginx、Redis、Muduo）的标准做法。用一块固定大小的内存，维护 `head` 和 `tail` 指针，头尾删除和追加都是 O(1)，且内存连续，缓存命中率高。**但实现略复杂。**



#### DAY12

##### 为什么要把hpp和cpp分开呢？

hpp相当与目录，cpp是具体实现代码

1. 在 `.cpp` 源文件中使用（最常见）

你写 `main.cpp` 想启动服务器，或者写 `UserSession.cpp` 要实现具体功能时，必须把相关的头文件包含进来。

-   **场景**：你在 `main.cpp` 里要创建 `ChatServer` 对象。
-   **做法**：`#include "ChatServer.hpp"`
-   **原因**：编译器在编译 `main.cpp` 时，必须先看到 `ChatServer` 的**声明**（它有什么构造函数、有什么 `Start()` 方法），否则它不知道 `new ChatServer()` 是否合法，会报 `unknown type` 错误。

2. 在另一个 `.hpp` 头文件中使用（继承或组合）

当你的头文件 **A** 使用了另一个头文件 **B** 里定义的类型时，必须把 **B** 包含进来。

-   **场景**：你的 `ChatServer.hpp` 内部有一个 `std::unique_ptr<MessageStore>` 成员变量，或者你的类继承了某个基类。
-   **做法**：在 `ChatServer.hpp` 顶部写 `#include "MessageStore.hpp"`（或 `#include <memory>`）。
-   **原因**：编译器在解析 `ChatServer` 的“蓝图”时，需要知道你写的 `MessageStore` 到底是个什么东西，占多大内存（虽然指针大小固定，但为了类型安全检查，依然需要完整声明）。

#####  std:uint64_t

这是一个**无符号的 64 位整数类型**，它的核心特性是：**无论在 Windows、Linux 还是 macOS 上编译，它都保证占用恰好 64 个比特位（8 个字节），取值范围是 0 ~ 18,446,744,073,709,551,615（约 1844 亿亿）**。

要你的代码里用了它，在任意平台上编译，内存大小和行为都完全一致。

| 使用场景                | 为什么用它？                                                 |
| :---------------------- | :----------------------------------------------------------- |
| **消息 ID（自增主键）** | 即使每秒发 1 万条消息，64 位整数也需要 **5 千多万年** 才会溢出，完全不用担心 ID 耗尽。 |
| **用户 ID（UID）**      | 支持全球几十亿用户，绰绰有余。                               |
| **时间戳（毫秒/微秒）** | 存储 Unix 毫秒时间戳（如 `1734567890123`），普通的 32 位 `int` 存不下，必须用 64 位。 |
| **文件传输大小**        | 记录一个 4GB 的大文件传输进度，用 64 位才能精确表示。        |



##### &

**使用 `&`（引用）最大的两个目的是：① 避免拷贝（省内存、省时间）；② 修改传入的原始数据。**

🥇 法则一：传入“大体积”数据，且只读 —— 用 `const T&`（常量引用）

🥈 法则二：需要在函数内部“修改”传入的变量 —— 用 `T&`（普通引用）

🥉 法则三：传入“小体积”数据（基本类型）—— 直接不用 `&`（按值传递）

| 参数类型                     | 是否使用 `&`       | 写法示例                                     | 理由                                        |
| :--------------------------- | :----------------- | :------------------------------------------- | :------------------------------------------ |
| **普通 `int` / `bool`**      | ❌ 不用             | `void SetId(int id)`                         | 复制成本极低，没必要用引用                  |
| **`std::string`（只读）**    | ✅ 必须用           | `void Log(const std::string& msg)`           | 避免复制 string 内部的字符数组（省内存）    |
| **`std::vector`（只读）**    | ✅ 必须用           | `void Process(const std::vector<int>& data)` | 避免复制整个 vector                         |
| **`Message` 结构体（只读）** | ✅ 必须用           | `void Save(const Message& msg)`              | 结构体有多个字段，复制开销大                |
| **`Message` 结构体（修改）** | ✅ 必须用           | `void FillHeader(Message& msg)`              | 需要在函数内修改并传出去                    |
| **`uint64_t`（只读）**       | ❌ 不用（直接用值） | `void Send(uint64_t id)`                     | 它只有 8 字节，和引用一样大，直接传值更简洁 |









##### `operator[]` 的“隐式插入”

你之所以能直接 `auto& conv = map[key]`，是因为 C++ 的 `std::unordered_map` 的 `operator[]` 被设计成：

>   如果 key 不存在，就等价于 `map.emplace(key, Value{})`。

这意味着：

-   如果 `Value` 是 `std::vector`，它会自动创建一个空向量。
-   如果 `Value` 没有默认构造函数（比如需要传参才能构造），这行代码就会**编译报错**。



##### std::transform

```c
std::transform(
    text.begin(),          // ① 输入区间的起始位置
    text.end(),            // ② 输入区间的结束位置（只处理这一段）
    text.begin(),          // ③ 输出结果的起始位置（这里和输入是同一个，就地修改）
    [](unsigned char ch) { // ④ 加工规则（一元操作函数）
        return static_cast<char>(std::toupper(ch));
    }
);
```

第四个参数使用了lamada表达式

等同于：

```c
auto it = text.begin();          // 输出位置的迭代器
for (auto it_in = text.begin(); it_in != text.end(); ++it_in, ++it) {
    *it = std::toupper(*it_in);  // 把加工结果写入输出位置
}
```

| 维度         | 手写 `for` 循环                                           | `std::transform`                                             |
| :----------- | :-------------------------------------------------------- | :----------------------------------------------------------- |
| **意图表达** | 编译器只看得到“你在循环”，具体做什么要读完循环体才懂。    | 看到 `transform` 就知道你在做“映射转换”，代码自文档化。      |
| **错误几率** | 容易写错迭代器自增、边界条件（比如少写 `+1`）。           | 标准库帮你管理迭代器，不会出错。                             |
| **性能优化** | 如果写成 `for (char c : text)` 并赋值，编译器也能优化好。 | 标准库实现通常高度优化，可能使用 SIMD 指令（单指令流多数据流）加速，在特定场景比手写更快。 |
| **扩展性**   | 如果要把结果输出到另一个容器，要写很多行。                | `transform(v1.begin(), v1.end(), back_inserter(v2), func);` 一行搞定。 |

#### DAY13

##### 密码哈希

###### 明文存储

如果直接吧密码放在结构体或者类里面，如果服务器被黑，那么里面所有的数据都会被泄漏---也就是明文密码，直接保存

###### 哈希

全世界共有一套哈希算法，输入一段固定字符，生成对应的固定的一段文本

1.   确定性：输入一样输出一样
2.   不可逆性：无法从输出推导输入

但是如果只用哈希，那么黑客就可以用这一套共有的算法去把常见的密码哈希表生成好，然后根据输出查输入---**彩虹表攻击**

###### 随即盐

如果说现在的哈希密码是由一套算法算出来的，也就是固定哈希密码，那么我们把“ 哈希（密码+随即盐）”这样的结果就是不固定的，而且还能让不同人的相同密码生成不一样的哈希密码，而随即盐则被明文存储的。

显而易见，彩虹表的那种方法必然失效的

现实中，盐其实不是密码，是可以公开储存的，他的作用不是加密，而是让我破解过程更加混乱

###### 慢哈希算法

那么，黑客仍然可以通过获得每一个用户的盐，然后开始 哈希（（猜测密码）+盐），每拼接一次就哈希一次，早期算法很快，但是现代算法故意设计的及其缓慢，，大量占用内存，一次就要0.1s,所以说，暴力破解也是不可取到密码的



##### 字节向量

在 C++ 里通常指 `std::vector<uint8_t>` 或 `std::vector<unsigned char>`

-   **字节（Byte）**：就是 8 个 bit，能表示 0~255 的整数。它是计算机存储的**最小基本单位**，不管是文字、图片、还是网络数据包，在内存里都是一个个字节。
-   **向量（Vector）**：就是 C++ 里的动态数组，可以随意在尾部追加数据，会自动扩容。

**合起来，字节向量就是“一个可以自动扩容的、专门用来存放原始二进制数据的数组”。**

###### 它和std::string有什么区别？

-   **`std::string` 的坑**：它默认把数据当成**字符序列**，并在很多底层实现中依赖 **`\0`（空字符）作为结束标志**。如果你的协议里需要发送二进制数据（比如文件传输、加密后的乱码），**而数据中恰好包含 `\0`，`string` 可能会截断数据**，导致数据丢失或乱码。
-   **`std::vector<uint8_t>` 的优势**：它是“无情的字节搬运工”。它**不关心**你存的是字母还是乱码，它只把每个字节当成 0~255 的数字存起来。**哪怕里面有 100 个 `\0`，它也照单全收，不会截断。**

所以说，因为盐和哈希值本质上都是二进制字节，里面可能有\0，所以要用字节向量

##### OPENSSL

###### 使用函数

1. **RAND_bytes：**   **生成盐**

    **函数原型**：

```c
#include <openssl/rand.h>
int RAND_bytes(unsigned char *buf, int num);
```

-   **参数详解**：
    -   `buf`：一个指向缓冲区的指针，用于存储生成的随机字节。**这个缓冲区必须已经分配好内存，且不能为 `NULL`**。
    -   `num`：请求生成的随机字节数。对于密码学应用，一个常见的值是 `16`（对应128位安全性）。
-   **返回值**：
    -   **成功时返回 `1`**。
    -   **失败时返回 `0`**。失败可能意味着系统的随机数发生器（熵源）不可用。**务必检查返回值**，不要假设它总能成功。

>   **补充**：还有一个 `RAND_priv_bytes()` 函数，语义相同，但旨在生成需要更高隐私保护的值（如长期密钥）。

---



2.**PKCS5_PBKDF2_HMAC**：产生哈希值

这是密码哈希的核心函数。它根据 **RFC 2898** 标准，通过加盐和多次迭代的方式，从一个密码中派生出固定长度的密钥（哈希值）。

-   **函数原型**：

    ```c
    #include <openssl/evp.h>
    int PKCS5_PBKDF2_HMAC(const char *pass, int passlen,
                          const unsigned char *salt, int saltlen,
                          int iter, const EVP_MD *digest,
                          int keylen, unsigned char *out);
    ```

-   **参数详解**：

    -   `pass`：指向明文字符串密码的指针。
    -   `passlen`：密码的长度。如果传入 `-1`，函数会自动用 `strlen()` 计算。
    -   `salt`：指向盐（salt）的指针。**这个参数不能为 `NULL`**，除非 `saltlen` 为 0。
    -   `saltlen`：盐的长度（字节数）。在你的场景中，应与 `RAND_bytes` 生成的字节数一致（例如 `16`）。
    -   `iter`：迭代次数。这是一个关键的安全参数，值越大，计算越慢，暴力破解的代价就越高。建议值应不低于 **1000**，现代应用如你的项目使用 `210000` 是合理的。
    -   `digest`：指定使用的哈希算法。在你的场景中应使用 `EVP_sha256()`。
    -   `keylen`：期望输出的密钥长度（字节数）。在你的场景中应为 `32`（对应256位）。
    -   `out`：指向输出缓冲区的指针，用于存储派生出的密钥（哈希值）。**这个缓冲区必须已经分配好，且大小至少为 `keylen` 字节**。

-   **返回值**：

    -   **成功时返回 `1`**。
    -   **失败时返回 `0`**。

    ---

    

3. **CRYPTO_memcmp**：常量时间内存比较

这个函数用于比较两块内存区域是否相等，其关键特性是**比较所花费的时间只取决于数据长度 `len`，而与具体的数据内容无关**。这能有效防止**时序攻击**。

-   **函数原型**：

    ```c
    #include <openssl/crypto.h>
    int CRYPTO_memcmp(const void *a, const void *b, size_t len);
    ```

-   **参数详解**：

    -   `a`：指向第一块内存区域的指针。
    -   `b`：指向第二块内存区域的指针。
    -   `len`：要比较的字节数。

-   **返回值**：

    -   **如果两块内存区域完全相同，返回 `0`**。
    -   **如果不同，返回一个非零值**。注意，这个非零值没有特殊含义，不能用于判断谁大谁小。

    

**我们为什么需要这个函数？？？**

首先，哈希值不是公开的

如果不使用这个函数的话，大多数编程语言和库在比较两块内存是否相同的时候，采用的都是**“短路逻辑”**：

-   从第一个字节开始，逐个字节比较。
-   一旦发现某个字节不同，**立即返回“不相等”**，停止继续比较。

所以说，对于攻击者，可以从第一个密码开始一个一个猜测，如果正确那么用时会比较长（因为还要检查后面的是否一样），如果错误，时间就会很短，这样子的话

**🛡️ `CRYPTO_memcmp` 如何解决？**

`CRYPTO_memcmp` 的设计目标就是**消除时间上的差异**：

-   它**不会提前返回**，而是**完整地比较完所有 `len` 个字节**，无论它们是否相同。
-   比较过程的所有循环迭代次数固定，只取决于 `len`，与数据内容无关。

这样子，无论怎么样，比较的时间都是一样的，攻击者就不能猜出来了



###### 注册时：生成并存储盐和哈希值

-   **Step 1：调用 `RAND_bytes`**
    -   **作用**：密码学安全的随机数生成器。你给它一个空缓冲区，它用操作系统底层的熵池填满它。
    -   **产出**：得到 **16 字节（128位）** 的随机二进制数据，这就是当前账号的**盐（Salt）**。
-   **Step 2：调用 `PKCS5_PBKDF2_HMAC`**
    -   **作用**：这是**密钥派生引擎**。它接收五个核心输入：明文密码、上一步生成的盐、固定高迭代次数（如210000）、哈希算法（如SHA256）。
    -   **过程**：它用盐和密码按照迭代次数进行复杂的重复运算。
    -   **产出**：得到 **32 字节（256位）** 的派生出哈希值。
-   **Step 3：（入库）**
    -   这里不调用 OpenSSL 函数。将 **Step 1 生成的原始盐** 和 **Step 2 生成的原始哈希** 直接以二进制形式写入数据库的 `salt` 和 `hash` 字段。

###### 登录时：验证哈希值

-   **Step 1：（出库）**
    -   不调用 OpenSSL。根据用户名查询数据库，直接拉取该账号的 **Salt（盐）**、**Hash（哈希）** 和 **Iterations（迭代次数）**。
    
-   **Step 2：再次调用 `PKCS5_PBKDF2_HMAC`**
    -   **作用**：这次作为**计算引擎**。你将用户本次输入的明文密码 + Step 1 从数据库读出的旧盐 + 读出的迭代次数 传入。
    -   **产出**：计算出一个新的 **32 字节候选哈希值**。
    
-   **Step 3：调用 `CRYPTO_memcmp`**
    -   **作用**：**常量时间内存比较器**。普通 `memcmp` 在第一个不同字节处就返回，攻击者可借此推测哈希值；`CRYPTO_memcmp` 保证比较时间永远是恒定的，防御时序攻击。
    -   **过程**：比较 Step 2 计算出的候选哈希 与 Step 1 从数据库拉取的老哈希 是否完全一致。
    
-   **Step 4：判定**
    -   如果 `CRYPTO_memcmp` 返回 0（相等），密码正确；返回非0，密码错误。
    
        
    
        
    
        
    
        

#### DAY14

##### SQL语句预处理

普通拼接SQL的话，若黑客向SQL语句后面拼接` OR '1'='1'`，那么前面的SQL就会暴露出来，这样子的话，数据库的数据就全部都暴露----**SQL注入**

而预处理的话，先固定一个模板，把指令的结构固定死，之后传数据，就算有恶意语句是跟上来，那也不是固定语句部分，只会被识别成普通文本

###### 准备阶段

这个阶段只发送结构，不发送数据

1.   客户端发送`mysql_stmt_prepare()`，客户端把SQL中有`？`的模板打包成网络包，发送给MYSQL服务器（具体的数据并没有发
2.   MYSQL服务器受到后，进行解析，拆成关键字、表名、字段名、占位符 ？，生成一个**解析树**，**那么这个解析树有什么作用？？**
     -   首先，服务器拿这个树去查数据字典，服务器去检验表在吗？有insert权限吗？字段是什么类型正确吗？
     -   推导参数类型，解析后把MYSQL要用的参数类型打包放进发送给客户端的元数据包，这样子客户端才能把数据类型转换成mysql需要的类型
3.   不仅生成了解析树，还有执行计划，因为看不到？的具体值，所以生成的一般是一份通用的执行计划，保存在服务器的`Statement Cache`，之后就不会用优化器，省CPU,缺点就是这不是最优解，而是一份万金油方案
4.   服务器返回的statementID,在服务端实际指向的是**解析树+执行计划+类型元数据**的符合内存对象
5.   最后给客户端返回一个网络包，里面有整数ID,字段的元数据

###### 执行阶段

1.   客户端调用`mysql_stmt_bind_param()`绑定数据，然后调用`mysql_stmt_execute()`，发送一个二进制的数据
     -   statementID：第一次服务端发过来是给客户端说，我解析好的语法树之类的标号为这个id,当客户端要取的时候，也就是客户端第二次向服务端发送这个id的时候，利用这个id就能很快的找到之前存储的地方，寻址作用。
     -   二进制格式数据：此时客户端已经转换成了二进制编码，传输效率很高
2.   MYSQL服务器收到之后，根据收到的ID直接命中之前缓存好的执行计划，把二进制数据填充到语法树的位置，然后执行计划，把数据写进磁盘buffer
3.   当客户端收到客户端转换成二进制格式的数据的时候，服务器只调用memcpy去填充字节，而不是使用SQL解析器解读这个字符串（可能会SQL注入），服务器通过ID区分那里是固定的指令，对于其他指令就当做参数而不是指令执行



###### 生命周期

1.   预处理语句是绑定在数据库连接上的，如果此时用mysqlclose断开连接，那么为释放的预处理语句就会被清理
2.   服务器缓存，第一次很慢，因为要解析，后面就会很快了



​			







#### DAY15

##### 运算符重载operator

###### 是什么？

是c++的一个关键字，和运算符一起使用，表示一个**运算符重载函数**，目的是为了扩展运算符的功能

-   重载之前和之后的使用方法一致
-   只能通过函数的方式实现

###### 为什么？

基本的运算符只能对一些基本的数据类型运算，但是重载之后，我们可以直接让自己定义的类进行操作运算

###### 怎么做？

-   实现为类的成员函数
-   实现为非类的成员函数（即全局函数

​           左操作数的参数类型必须被显式指定

**怎么选择呢？**

-   如果说与他一起使用的左操作数是类的对象就使用第一种，但是是其他的类型，就要重载为全局函数
-   “=，[],(),->”必须被定义为类的成员函数，不然会编译报错

###### 限制

1.  重载后的运算符的操作数至少有一个是用户定义的类型
2.  不能违法原运算符的语法规则，也就是使用方法一致
3.  不能创建新的运算符
4.  有一些不能重载比如说sizeof

```c++
void operator()(MYSQL_STMT* statement) const {
        if (statement != nullptr) {
            mysql_stmt_close(statement);
        }
    }//重载了（）
```



##### 页分裂

1.   插入前的检查：

当一个插入操作被发起，首先，InnoDB会定位到要插入记录的位置。这通常涉及到B+树索引的搜索。
在找到插入点后，InnoDB检查目标页是否有足够空间容纳新记录。这一步是通过比较页内剩余空间和新记录大小来完成的。

2.   判断是否需要页分裂：

如果页内有足够空间，则直接插入记录，无需页分裂。
如果没有足够空间，InnoDB决定进行页分裂。

3.   执行页分裂：

创建新页：系统分配一个新的页，这个页的数据结构与被分裂的页相同。
记录的迁移和分配：选择一部分记录从原页迁移到新页。InnoDB试图平均分配两个页的空间利用，同时保持B+树的顺序性不变。
插入新记录：根据新记录的键值决定它是插入到原页还是新页中。
调整B+树索引：页分裂可能导致父节点的键值需要更新，以反映新页的加入。如果父节点没有足够空间，这个过程可能递归到更高层次的节点，甚至到根节点，可能导致树的高度增加。

4.   更新系统元数据和日志：

元数据更新：包括页的元数据，如页链表、空闲列表等。
写入重做日志（Redo Log）：每次页分裂操作都会记录在重做日志中，确保在系统故障时可以恢复数据。

##### 表的创建

###### 第 1 层：毛坯房（你最熟悉的骨架）

任何创建表的原始骨架都是这样的：

sql

```
CREATE TABLE users (
    列1名字 数据类型,
    列2名字 数据类型
);
```



你的代码里，最核心的骨架其实就长这样（我帮你抽出来）：

sql

```
CREATE TABLE users (
    id 数据类型,
    username 数据类型,
    password_salt 数据类型,
    password_hash 数据类型,
    password_iterations 数据类型,
    created_at 数据类型,
    updated_at 数据类型
);
```



**看到没？本质上就是给这张表定义了 7 个列（字段）而已！**

------

###### 第 2 层：给“格子”贴标签（你看不懂的修饰词）

你现在看不懂的，全是**修饰这 7 个列的“额外要求”**。我们逐个翻译成大白话：

| 你看不懂的代码                    | 大白话翻译                                                   |
| :-------------------------------- | :----------------------------------------------------------- |
| **`BIGINT`**                      | 这个格子用来存**超大整数**（比如用户 ID 能存到几百亿）。     |
| **`UNSIGNED`**                    | 这个格子**不能填负数**（ID 没有负的）。                      |
| **`NOT NULL`**                    | 这个格子**必须填内容**，不能留空。                           |
| **`AUTO_INCREMENT`**              | 这个格子**自动编号**。你不需要填，插入新用户时它自动变 1, 2, 3... |
| **`VARCHAR(20)`**                 | 这个格子存**不超过 20 个的变长文字**（比如用户名）。         |
| **`CHARACTER SET ascii`**         | 这个格子只允许**英文字母和数字**，不许存中文（防止有人起中文名导致索引变慢）。 |
| **`VARBINARY(16)`**               | 这个格子存**二进制乱码**（专门用来存密码的“盐”，因为盐不是给人看的文字）。 |
| **`DEFAULT CURRENT_TIMESTAMP`**   | 如果你不填这个格子，**系统自动填上当前时间**。               |
| **`ON UPDATE CURRENT_TIMESTAMP`** | 只要这一行数据被修改，这个格子就**自动更新为最新时间**。     |

------

###### 第 3 层：给表格立规矩（最后的约束）

你看不懂的最后两行，是在给这张表**立规矩**：

| 你看不懂的代码                                | 大白话翻译                                                   |
| :-------------------------------------------- | :----------------------------------------------------------- |
| **`PRIMARY KEY (id)`**                        | 拿 `id` 列当**身份证号**（主键），靠它快速找到某一个人。     |
| **`UNIQUE KEY uq_users_username (username)`** | 给 `username` 列加个**防重复锁**。如果有人注册 `张三`，你再插 `张三`，数据库直接报错，不让你插进去。 |
| **`ENGINE=InnoDB`**                           | 指定用 **InnoDB** 存储引擎（你可以理解为：这张表格必须用“高级纸质”来做，支持回滚、支持并发），这是 MySQL 默认且最推荐的。 |

##### 登录流程

![image-20260731205528245](/home/white/.config/Typora/typora-user-images/image-20260731205528245.png)

```
客户端（你的应用）                          MySQL 服务端
     |                                            |
     |-------- (1) PREPARE SQL 文本 ------------> |  语法解析，生成 ID
     | <------ (1) 返回 Statement ID ----------- | 
     |                                            |
     | [bind_param] 纯本地操作，无网络              |
     |                                            |
     |-------- (2) EXECUTE (ID + 参数值) -------> |  优化器跑起来！执行！
     | <------ (2) 返回结果集头 / OK -----------  |  数据暂时留在服务端
     |                                            |
     |-------- (2) 服务端推送结果集数据 ---------> |  (这一步算在第二次往返的数据流中)
     |                                            |
     | [store_result + bind_result] 纯本地内存操作 |
     | [fetch] 纯本地内存读取，无网络             |
     |                                            |
     |-------- (3) CLOSE (ID) -------------------> |  清理缓存，释放 ID
     | <------ (3) 关闭成功 --------------------- | 
     v                                            v
```

**确保数据库连接有效**（`ensure_connected`）

-   若连接断开则自动重连。

1.  **准备 SQL 预处理语句**（`prepare_statement`）
    -   调用 `mysql_stmt_init` 创建句柄。
    -   调用 `mysql_stmt_prepare` 解析 SQL（带 `?` 占位符）。
    -   若失败，返回错误。
2.  **绑定查询参数（用户名）**
    -   创建 `MYSQL_BIND` 结构体，类型为 `MYSQL_TYPE_STRING`，缓冲区指向 `username.c_str()`。
    -   调用 `mysql_stmt_bind_param` 把用户名绑定到 `?`。
3.  **执行查询**（`mysql_stmt_execute`）
    -   MySQL 根据用户名查找 `users` 表。
4.  **缓存结果集**（`mysql_stmt_store_result`）
    -   将查询到的所有行（实际因为 `LIMIT 1` 最多一行）拉到客户端内存，以便后续读取。
5.  **绑定结果列到 C++ 变量**（`mysql_stmt_bind_result`）
    -   分别绑定 salt 缓冲区（`salt_buffer`）、hash 缓冲区（`hash_buffer`）、迭代次数变量（`iterations`）。
    -   设置 `length` 数组来接收实际数据长度，`is_null` 和 `error` 用于检查数据有效性。
6.  **获取第一行数据**（`mysql_stmt_fetch`）
    -   若返回 `MYSQL_NO_DATA`，说明用户名不存在 → 返回 `InvalidCredentials`（统一错误信息，不暴露账号是否存在）。
    -   若发生截断或错误（`MYSQL_DATA_TRUNCATED`、非零返回值、或 `is_null` / `error` 置位）→ 返回 `Error`（数据损坏）。
    -   若成功获取，则提取出盐、哈希、迭代次数。
7.  **组装 `PasswordRecord`**
    -   用 `lengths[0]` 和 `lengths[1]` 确定实际字节数，从缓冲区复制到 `std::vector<unsigned char>`。
8.  **调用 `PasswordHasher::verify`**
    -   传入用户输入的密码和数据库中取出的记录，进行 PBKDF2 再次派生和常量时间比较。
9.  **返回结果**
    -   若验证成功，返回 `VerifyUserResult::Success`，否则 `InvalidCredentials`。
    -   任何数据库异常均返回 `Error`（并填充 `error` 字符串）。

###### 安全设计亮点

-   **使用预处理语句**：防止 SQL 注入。
-   **二进制数据直接存储**：盐和哈希用 `VARBINARY`，避免字符集问题。
-   **错误信息统一**：用户名不存在和密码错误都返回 `InvalidCredentials`，防止账户枚举攻击。
-   **资源自动释放**：使用 RAII（`std::unique_ptr`）确保语句句柄被释放。
-   **防御性检查**：检查 `is_null`、`error`、截断等，确保数据完整性。



#### DAY16

#####  数据库交互流程

###### 路线一：写入/上传流程（客户端 → 服务器 → MySQL）

以 **`REGISTER alice pass123`（注册新账号）** 为例：

| 步骤 | 所在进程             | 函数名                                                       | 具体动作                                                     |
| :--- | :------------------- | :----------------------------------------------------------- | :----------------------------------------------------------- |
| 1    | **客户端**           | `main()` → `send_all()`                                      | 用户输入 `REGISTER alice pass123`，客户端通过 `send()` 把文本发到服务端 socket。 |
| 2    | **服务端**           | `ChatServer::run()` → `epoll_wait()`                         | 服务端收到数据，触发 `EPOLLIN` 事件。                        |
| 3    | **服务端**           | `ChatServer::handle_client_read()`                           | 从 socket `recv()` 数据，追加到 `in_buffer`，按 `\n` 切分出完整行 `"REGISTER alice pass123"`。 |
| 4    | **服务端**           | `ChatServer::handle_command()`                               | 调用 `parse_command()` 解析出 `name="REGISTER"`，进入 `else if (command.name == "REGISTER")` 分支。 |
| 5    | **服务端**           | `ChatServer::handle_register()`                              | 校验用户名密码格式、检查是否已登录；然后调用 `user_repository_.create_user(username, password, error)`。 |
| 6    | **服务端（仓库层）** | `MySqlUserRepository::create_user()`                         | 调用 `PasswordHasher::create()` 生成随机盐和 PBKDF2 哈希；准备 SQL：`INSERT INTO users (...) VALUES (?, ?, ?, ?)`。 |
| 7    | **服务端（仓库层）** | `ensure_connected()` → `prepare_statement()` → `mysql_stmt_bind_param()` → `mysql_stmt_execute()` | 执行预处理语句，把用户名、盐、哈希、迭代次数写入 MySQL。     |
| 8    | **服务端**           | 逐层返回                                                     | 如果成功，`create_user` 返回 `Success`，`handle_register` 调用 `queue_message()` 把 `"[system] registration successful"` 放入客户端的 `out_buffer`。 |
| 9    | **服务端**           | `ChatServer::handle_client_write()`                          | 在下一个事件循环中，通过 `send()` 把 `out_buffer` 中的成功消息发回给客户端。 |
| 10   | **客户端**           | `main()` 中的 `recv()`                                       | 收到服务器回显的文本，打印在终端上。                         |

**关键结论**：写入流程必走 **“业务校验 → 仓库层 → 预处理语句 → 提交”**。

------

###### 路线二：读取/下载流程（MySQL → 服务器 → 客户端）

以 **`WHO`（查看在线用户列表）** 为例：

**注意**：`WHO` 读取的是 **`online_users_` 内存表**（由服务端维护，不直接查 MySQL）。但如果要查 **“好友列表（`FRIENDS`）”**，它读的是服务启动时缓存的 `users_` 内存 map，**也不是实时查 MySQL**。

为了让看清楚 **“真正实时查 MySQL 的读取”**，我们以 **`HISTORY_PRIVATE bob 10`（拉取私聊历史）** 为例（假设 v7.2 以后消息已迁移到 MySQL）：

| 步骤 | 所在进程             | 函数名                                                       | 具体动作                                                     |
| :--- | :------------------- | :----------------------------------------------------------- | :----------------------------------------------------------- |
| 1    | **客户端**           | `main()` → `send_all()`                                      | 用户输入 `HISTORY_PRIVATE bob 10`，发给服务端。              |
| 2    | **服务端**           | `ChatServer::handle_command()`                               | 路由到 `handle_private_history()`。                          |
| 3    | **服务端**           | `ChatServer::handle_private_history()`                       | 校验登录、解析参数（`bob` 和 `10`），然后调用 `message_store_.recent_private(...)`。 |
| 4    | **服务端**           | `InMemoryMessageStore::recent_private()`（如果是内存存储）或 `MySqlMessageRepository::load_recent_private()`（如果是 MySQL 存储） | 如果是 MySQL 存储，执行 `SELECT sender, recipient, content, created_at FROM private_messages WHERE (sender = ? AND recipient = ?) OR ... ORDER BY id DESC LIMIT ?`。 |
| 5    | **服务端（仓库层）** | `ensure_connected()` → `mysql_query()` 或 `mysql_stmt_prepare()` → `mysql_stmt_execute()` → `mysql_store_result()` | 把数据从 MySQL 拉取到服务端内存的 `MYSQL_RES*` 结果集中。    |
| 6    | **服务端**           | `mysql_fetch_row()` 循环                                     | 逐行取出消息内容，构造成 `ChatMessage` 对象，放进 `std::vector`。 |
| 7    | **服务端**           | `ChatServer::handle_private_history()`                       | 把 `vector` 中的消息格式化成文本（`#123 2025-01-01 alice -> bob: hello`），调用 `queue_message()` 放入 `out_buffer`。 |
| 8    | **服务端**           | `ChatServer::handle_client_write()`                          | 发回给客户端。                                               |
| 9    | **客户端**           | `recv()`                                                     | 打印显示。                                                   |

**关键结论**：读取流程必走 **“业务层 → 仓库层 → SQL 执行 → 结果集逐行解析 → 格式化回传”**。

------

###### 路线三：启动时批量加载（MySQL → 服务器内存）

这是最容易被忽略的一步。服务端**不是每次查好友关系都去 MySQL**，而是**启动时一次性全部读到内存**。

以 **`ChatServer::initialize()`** 加载好友关系为例：

| 步骤 | 函数名                                  | 动作                                                         |
| :--- | :-------------------------------------- | :----------------------------------------------------------- |
| 1    | `ChatServer::initialize()`              | 调用 `load_registered_users()`。                             |
| 2    | `ChatServer::load_registered_users()`   | 调用 `user_repository_.load_usernames(usernames, error)`。   |
| 3    | `MySqlUserRepository::load_usernames()` | 执行 `SELECT username FROM users`，把所有用户名读进 `vector`，然后 `users_.emplace(username, UserAccount{...})` 放入服务器内存。 |
| 4    | `MySqlFriendRepository::load_state()`   | 执行 `SELECT ... FROM friendships` 和 `SELECT ... FROM friend_requests`，填进 `state.friendships` 和 `state.pending_requests`，供 `are_friends` 查询使用。 |

**关键结论**：**好友关系、账号列表**在启动时全量缓存。`WHO`、`FRIENDS`、`are_friends` 走的都是内存，**不实时查 MySQL**，只有 `HISTORY` 或 `REGISTER`/`LOGIN` 等需要持久化存储/校验的操作才实时访问数据库。

------

###### 核心总结：函数名速查表

| 动作         | 客户端函数   | 服务端网络层函数                            | 服务端业务层函数                            | 仓库层函数                                   | MySQL API 函数                                           |
| :----------- | :----------- | :------------------------------------------ | :------------------------------------------ | :------------------------------------------- | :------------------------------------------------------- |
| 上传（写入） | `send_all()` | `handle_client_read()` → `handle_command()` | `handle_register()` / `handle_add_friend()` | `create_user()` / `create_request()`         | `mysql_stmt_prepare` → `bind` → `execute` + `commit`     |
| 下载（读取） | `recv()`     | `handle_client_write()`                     | `handle_public_history()`                   | `load_recent_private()` / `load_usernames()` | `mysql_query` + `mysql_store_result` + `mysql_fetch_row` |
| 启动缓存     | 无           | 无                                          | `initialize()`                              | `load_state()` / `load_usernames()`          | 同上                                                     |

**牢记这条铁律**：**客户端只发文本命令，只收文本回显；所有 SQL 查询和写入都封装在 `MySqlXxxRepository` 里，由服务端在接收到命令后调用。**



#### DAY17

##### BLOB数据

**BLOB** 是 **Binary Large OBject（二进制大对象）** 的缩写。在 MySQL 数据库中，它是一种专门用来存储**原始二进制数据**的数据类型。

-   **存的是什么**：存的是**字节流**（即 `0x00` 到 `0xFF` 的任意序列）。
-   **不做什么**：MySQL **不会**对它进行字符集转换（比如不会转 utf8），也**不会**因为里面包含 `\0` 空字符而截断它。它把数据当作“无意义的字节”原样存入、原样读出。

###### `BLOB` 和 `VARBINARY` 的区别（MySQL 特有）

-   **`BLOB`**：可变长，最大可达 **65KB**（`TINYBLOB` 到 `LONGBLOB` 不等），没有指定长度的语法（如 `BLOB(16)` 是无效的）。
-   **`VARBINARY(M)`**：指定最大长度 `M`（如 `VARBINARY(16)`），限制在 1~65535 字节之间，且会占用额外的长度字节。

在你的场景中，因为盐和哈希长度固定（16 和 32），用 **`VARBINARY(16)`** 和 **`VARBINARY(32)`** 是最精准、最节省空间的写法，相当于“固定长度的二进制列”。

##### 事务

**事务的本质是：一组必须“要么全部成功，要么全部失败”的数据库操作集合**

**在你的代码中，这个“事务”只包含下面这两条写操作：**

1.  **第一条 SQL**（往 `friend_requests` 表插数据）：

    

    ```sql
    INSERT INTO friend_requests (sender_user_id, receiver_user_id) 
    SELECT ... WHERE sender.username = 'alice' AND receiver.username = 'bob';
    ```

    

2.  **第二条 SQL**（往 `friend_events` 表插二进制事件）：

    

    ```sql
    INSERT INTO friend_events (actor_user_id, target_user_id, payload) 
    SELECT ... WHERE actor.username = 'alice' AND target.username = 'bob';
    ```

    

**“事务”就是这两条 SQL 的捆绑包**。
数据库（MySQL）会把这俩视为 **“一个完整的任务”**。



>**“事务”不是一个死的东西（不是一张表，也不是一个文件），它是一个活的状态，是你从 `begin`（起笔）到 `commit`（落款）这段时间内，MySQL 为你保留的一张“随时可以撕掉重来的草稿纸”。**



###### **为什么必须要“事务”？（没有它会出什么问题？）**

如果不加事务（即默认的“自动提交”模式），执行过程是这样的：

1.  执行 `INSERT INTO friend_requests` → **成功！数据立刻写进硬盘了。**
2.  执行 `INSERT INTO friend_events` → **失败了！**（比如 `protobuf_event` 太大，或者网络瞬间卡顿）。

**后果**：数据库里多了一条“好友请求记录”，但缺少了对应的“审计日志”。
你的业务逻辑（`ChatServer`）查好友请求时发现有记录，但查事件时发现没有，**数据产生不一致（脏数据）**，排错极其困难。

**有了事务**：
如果第二条 SQL 失败了，`rollback_transaction()` 会被调用，MySQL 会**瞬间把第一条 SQL 插入的数据删除掉**。
数据库最终恢复到执行第一条 SQL 之前的“空白状态”，仿佛什么都没发生过。

###### 事务”在底层到底是怎么执行的？（揭开黑盒）

**事务并不是把数据直接写在硬盘上，而是写在了“草稿纸”上。**

-   **`begin_transaction()`（开启）**：
    告诉 MySQL：“从现在起，把我要改的数据先写在**内存缓存（Buffer Pool）** 和 **撤销日志（Undo Log）** 里，不要直接动硬盘里的正式数据。”
-   **执行 SQL 期间（`INSERT`）**：
    数据在内存里被修改了。MySQL 专门记录了一份“旧数据”在 Undo Log 里，以防万一需要撤销。
-   **`commit_transaction()`（提交）**：
    告诉 MySQL：“我的草稿确定了！” MySQL 立刻把修改记录写入 **Redo Log（重做日志）** 并强制刷到硬盘。
    **此时，数据才算“永久固化”了**。即使下一秒电脑断电，重启后 MySQL 也能根据 Redo Log 把这笔修改恢复回来。
-   **`rollback_transaction()`（回滚）**：
    告诉 MySQL：“我反悔了！” MySQL 直接丢弃内存中的修改，然后**照着 Undo Log 里的旧数据，把内存恢复成原来的样子**。硬盘上的正式数据纹丝未动。

##### SQL

###### 1. 

```mysql
constexpr const char* kSql =
        "INSERT INTO friend_events ("
        "actor_user_id, "
        "target_user_id, "
        "payload"
        ") "
        "SELECT actor.id, target.id, ? "
        "FROM users actor "
        "JOIN users target ON 1 = 1 "
        "WHERE actor.username = ? "
        "AND target.username = ?";
```

这条 SQL 是想往 `friend_events`（好友事件）表里插一条记录。
这条记录要包含三个东西：

-   **谁干的**（操作者 ID）
-   **对谁干的**（目标用户 ID）
-   **干了什么事**（比如 `"ADD_FRIEND"`）

2. 难点在哪？

难点在于：上层业务传过来的是**用户名**（比如 `"alice"` 和 `"bob"`），但数据库存关系只认**数字 ID**（比如 `id=1` 和 `id=2`）。
所以必须把“用户名”转换成“ID”。

我们拆成三步来看：

第一步：给你手里的数据起外号**



```sql
FROM users actor
JOIN users target ON 1 = 1
```



这句话就是：**“把 users 表复制出两份，一份叫 actor（操作者），一份叫 target（目标）”**。

第二步：在复制品里筛出你要的那两个人**

```
WHERE actor.username = ? AND target.username = ?
```



比如你绑定的参数是 `actor = 'alice'`，`target = 'bob'`，那这里的结果就是：

| actor.id | actor.username | target.id | target.username |
| :------- | :------------- | :-------- | :-------------- |
| **1**    | alice          | **2**     | bob             |

第三步：把筛出来的数据插进去**

sql

```
SELECT actor.id, target.id, ?
```



就是把上面这一行里面的 `1`、`2`、和事件类型（`payload`）一起插进 `friend_events` 表。

###### 2.

```sqlite
constexpr const char* kFriendRequestsSql =
        "CREATE TABLE IF NOT EXISTS "
        "friend_requests ("
        "sender_user_id BIGINT UNSIGNED "
        "NOT NULL,"
        "receiver_user_id BIGINT UNSIGNED "
        "NOT NULL,"
        "created_at TIMESTAMP(3) NOT NULL "
        "DEFAULT CURRENT_TIMESTAMP(3),"
        "PRIMARY KEY ("
        "sender_user_id, receiver_user_id"
        "),"
        "KEY idx_friend_requests_receiver "
        "(receiver_user_id, created_at),"
        "CONSTRAINT fk_friend_requests_sender "
        "FOREIGN KEY (sender_user_id) "
        "REFERENCES users(id) "
        "ON DELETE CASCADE,"
        "CONSTRAINT fk_friend_requests_receiver "-- 强制要求 sender_user_id 必须存在于 users 表中
        "FOREIGN KEY (receiver_user_id) "
        "REFERENCES users(id) "
        "ON DELETE CASCADE,"-- 如果发送者用户被删除了，所有他发出去的申请会自动删除
        "CONSTRAINT chk_friend_request_users "-- 从根本上禁止用户给自己发好友申请
        "CHECK ("
        "sender_user_id <> receiver_user_id"
        ")"
        ") ENGINE=InnoDB";
```

###### 3.

```sql
constexpr const char* kSql =
        "SELECT event.id, event.payload "
        "FROM friend_events event "
        "JOIN users current_user "-- 从users表中查处用户的数字ID
        "ON current_user.username = ? "
        
        "WHERE event.actor_user_id = "
        "current_user.id "
        "OR event.target_user_id = "
        "current_user.id "-- 不管是发送者还是接受者都查询
        "ORDER BY event.id DESC "-- 事件ID降序排列
        "LIMIT ?";-- 查询行数
```

FROM后面是两张表，和为他们起的别名

ON后面的是连接条件，这两个表应该怎么关联在一起，从 `users` 表里，找出 `username` 字段等于我传入的那个字符串（比如 `'alice'`）的那一行，把它作为 `current_user`。”

###### 4.

```c
    constexpr const char* kSql =
        "DELETE friendship "
        "FROM friendships friendship "
        "JOIN users actor ON 1 = 1 "
        "JOIN users target ON 1 = 1 "
        "WHERE actor.username = ? "
        "AND target.username = ? "
        "AND friendship.user_id_low = "
        "LEAST(actor.id, target.id) "
        "AND friendship.user_id_high = "
        "GREATEST(actor.id, target.id)";
```

1. 为什么要这样写？（核心痛点）

在你的 `friendships` 表设计中，为了确保“好友关系”的唯一性，你强制规定：

-   **`user_id_low`** 存放 **较小的** 用户 ID
-   **`user_id_high`** 存放 **较大的** 用户 ID

这样做的好处是：不管 A 加 B，还是 B 加 A，存进数据库的都是 `(low_id, high_id)` 这一种顺序，不会出现 `(1,2)` 和 `(2,1)` 两条重复数据。

但是，当业务层（比如 `REMOVE_FRIEND alice bob`）传过来的是**用户名**时，你面临两个问题：

1.  要把 `"alice"` 和 `"bob"` 变成 `id`。
2.  要知道 `alice` 和 `bob` 谁的 `id` 更小，才能去匹配 `user_id_low`。

传统笨办法需要 **3 次 SQL 交互**：

1.  查 alice 的 id
2.  查 bob 的 id
3.  在 C++ 里用 `if` 判断大小，然后拼 SQL `DELETE ... WHERE low = ? AND high = ?`

2. 这条 SQL 如何“一步到位”？

它利用 `JOIN` 和 `LEAST/GREATEST` 函数，把上面 3 步**合并成了 1 步**：

-   **`JOIN users actor ON 1 = 1` 和 `JOIN users target ON 1 = 1`**
    先把 `users` 表复制成两份，通过 `WHERE actor.username = ? AND target.username = ?` 精准锁定那两个用户的行。
-   **`LEAST(actor.id, target.id)` 和 `GREATEST(actor.id, target.id)`**
    直接在 SQL 里计算出谁小谁大，无需 C++ 代码插手。然后用这两个计算结果去匹配 `friendships` 表中的 `user_id_low` 和 `user_id_high`。

3. 完整的执行逻辑拆解

假设数据库里有：

-   `alice` 的 `id = 10`
-   `bob` 的 `id = 5`

当你执行 `REMOVE_FRIEND alice bob` 并绑定这两个用户名时，SQL 内部的计算过程是这样的：

| 步骤 | 逻辑                                        | 结果               |
| :--- | :------------------------------------------ | :----------------- |
| 1    | 根据 `actor.username` 查出 `actor.id`       | `10`               |
| 2    | 根据 `target.username` 查出 `target.id`     | `5`                |
| 3    | 计算 `LEAST(10, 5)`                         | **`5`**            |
| 4    | 计算 `GREATEST(10, 5)`                      | **`10`**           |
| 5    | 去 `friendships` 表里找 `low=5 AND high=10` | 找到这条记录并删除 |

4. 致命陷阱（必须注意）

和之前的 `INSERT` 一样，这条 SQL **不会因为用户不存在而报错**。如果 `alice` 或 `bob` 根本不在 `users` 表里，`JOIN` 查不出数据，**这条 SQL 依然“成功执行”，但影响行数是 0**。

因此，你的 C++ 代码**必须**检查 `mysql_stmt_affected_rows()`：

```
if (mysql_stmt_affected_rows(statement.get()) != 1) {
    error = "User does not exist or they are not friends";
    return false;
}
```



###### 5.

```sql
CREATE TABLE IF NOT EXISTS messages (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    message_type TINYINT UNSIGNED NOT NULL,
    sender_username VARCHAR(20) NOT NULL,
    recipient_username VARCHAR(20) NULL,
    created_at_unix_ms BIGINT NOT NULL,
    payload LONGBLOB NOT NULL,
    PRIMARY KEY (id),
    INDEX idx_messages_public (message_type, id),
    INDEX idx_messages_sender_recipient (
        message_type,
        sender_username,
        recipient_username,
        id
    ),
    INDEX idx_messages_recipient_sender (
        message_type,
        recipient_username,
        sender_username,
        id
    ),
    CONSTRAINT fk_message_sender
        FOREIGN KEY (sender_username) REFERENCES users(username)
        ON DELETE CASCADE,
    CONSTRAINT fk_message_recipient
        FOREIGN KEY (recipient_username) REFERENCES users(username)
        ON DELETE CASCADE,
    CONSTRAINT chk_message_type CHECK (message_type IN (1, 2)),
    CONSTRAINT chk_message_recipient CHECK (
        (message_type = 1 AND recipient_username IS NULL) OR
        (message_type = 2 AND recipient_username IS NOT NULL)
    )
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

```

2. 核心字段拆解：为什么存这些？

-   **`id`**
    自增主键。**物理排序依据**。不管时间准不准，`id` 越大的消息一定是越晚写入的。在查询“最近 N 条消息”时，直接用 `ORDER BY id DESC` 性能最快，不需要额外的时间索引。
-   **`message_type TINYINT UNSIGNED`**
    消息类型（1=公共消息，2=私聊消息）。之所以用 `TINYINT`（1字节）而不存字符串，是为了节省存储空间和索引体积。
-   **`sender_username` 和 `recipient_username`**
    直接存用户名（字符串），而不是存用户 ID。
    这是一个**反常规设计**。大多数系统会存 ID 以节省空间，但这里存用户名有一个巨大好处：**查询历史记录时，不需要 JOIN `users` 表去查名字，直接读出来就能用**，减少了联表查询的开销。代价是用户名如果改了，历史消息里的名字不会变（但用户名通常不允许修改，所以没问题）。
-   **`created_at_unix_ms BIGINT`**
    存 **Unix 毫秒时间戳**（整数），而不是 MySQL 的 `TIMESTAMP`。
    好处：不受时区影响，不受 2038 年影响，C++ 的 `std::chrono` 可以直接转成这个数字，省去了 `TIMESTAMP` 格式化转换的开销。
-   **`payload LONGBLOB`**
    这是整张表**最核心、最灵活**的设计。
    没有定义 `content`、`image_url`、`file_size` 这些乱七八糟的列，而是把所有消息内容**打包成一个 Protobuf 二进制流**塞进 `LONGBLOB`。
    为什么？因为聊天消息的格式会变（以后可能加表情、引用回复、@提醒、语音），如果加一列就要改一次表结构，太麻烦。用 `payload` 存 Protobuf，**消息格式的升级完全由代码控制，数据库不需要跟着改**。

3. 约束：让数据库当“警察”，拦截非法数据

-   **`chk_message_type CHECK (message_type IN (1, 2))`**
    限制消息类型只能是 1 或 2。如果 C++ 代码出了 BUG，想插入一个 3，数据库会直接拒绝，不会污染数据。
-   **`chk_message_recipient CHECK (...)`**
    这是这张表**最惊艳的约束**。它强制要求：
    -   如果 `message_type = 1`（公共消息），`recipient_username` **必须为 NULL**。
    -   如果 `message_type = 2`（私聊消息），`recipient_username` **必须不为 NULL**。
        这个约束在数据库层面**硬性锁死了“公共消息没有接收者，私聊必须有接收者”的业务规则**。即使你写 C++ 时忘了赋值，MySQL 也会报错，相当于多了一道安全防线。

4. 外键：用户注销了，消息怎么办？

-   **`fk_message_sender`** 和 **`fk_message_recipient`**
    引用了 `users(username)`，并且 `ON DELETE CASCADE`。
    意思是：如果用户注销（`users` 表删除了该行），**他发的所有消息和他收到的所有私聊，都会被 MySQL 自动删除**。
    这保证了数据库里不会留下“查无此人”的孤儿消息，省去了你写 C++ 代码去清理的麻烦。

5. 索引：为了让查询“飞”起来

你的内存消息存储只有几百条，不需要索引。但现在消息要存硬盘，可能有几百万条，所以索引是关键。

-   **`idx_messages_public (message_type, id)`**
    专门为 `FRIEND_EVENTS`（实际上应该是 `HISTORY_PUBLIC 20`）命令设计的。
    查询条件是 `WHERE message_type = 1 ORDER BY id DESC LIMIT 20`，走这个索引直接定位到公共消息的尾部，瞬间完成。
-   **`idx_messages_sender_recipient (message_type, sender_username, recipient_username, id)`**
    专门为查询 **“我发给某个人的历史消息”** 设计的。
    查询条件是 `WHERE message_type = 2 AND sender_username = 'alice' AND recipient_username = 'bob' ORDER BY id DESC`，索引直接命中三列，无须回表。
-   **`idx_messages_recipient_sender (message_type, recipient_username, sender_username, id)`**
    专门为查询 **“某人发给我的历史消息”** 设计的（方向和上面相反）。
    因为私聊是双向的，你不能保证客户端的查询方向，所以建了两个相反方向的索引，**无论从哪个角度查，都能用上索引**。

6. 存储引擎和字符集

-   **`ENGINE=InnoDB`**
    支持事务、支持外键、支持行级锁，高并发下比 MyISAM 稳定得多。
-   **`DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin`**
    表默认字符集是 `utf8mb4`（支持表情符号），排序规则是 `utf8mb4_bin`（区分大小写）。
    但是，`sender_username` 和 `recipient_username` 字段**必须**沿用 `users` 表的 `ascii` 字符集，否则外键会报错（因为字符集不一致）。这一点是你建表时需要留意的，但属于物理实现细节，不影响你理解它的逻辑设计。



































##### 函数

###### ROLLBACK

```c#
//取消当前事务中从BEGIN以来执行d所有未提交的数据库修改，让数据库回到事务开始的状态
void MySqlFriendRepository::rollback_transaction() {
    if (connection_ != nullptr) {
        mysql_rollback(connection_);//服务器向MYSQL发送一个ROLLBACK包
    }
}
```

 MySQL 服务器收到 `ROLLBACK` 后干了什么？

1.  **丢弃缓冲池修改**：把之前 `mysql_stmt_execute` 写入 InnoDB 内存缓冲池（Buffer Pool）但尚未落盘的数据，全部清除。
2.  **撤销 undo log**：利用 InnoDB 的 undo 日志，把已经被修改但尚未提交的数据行，恢复到事务开始前的旧值（比如删除 `friend_requests` 临时插入的记录）。
3.  **释放行锁**：释放之前 `INSERT` 或 `SELECT ... FOR UPDATE` 时持有的行锁或间隙锁，让其他事务（比如其他客户端）能够继续操作这些数据。
4.  **丢弃 Redo Log**：清空该事务在 redo log 中生成的待持久化记录，确保这些修改永远不会被刷入 `.ibd` 物理文件。



只要你看到 `rollback_transaction()`，就说明**前面某一步出错了**

```c
if (!statement) {
    rollback_transaction();  // ✅ SQL 预处理失败，撤销一切
    return Error;
}

if (mysql_stmt_bind_param(...) != 0) {
    rollback_transaction();  // ✅ 参数绑定失败，撤销一切
    return Error;
}

if (mysql_stmt_execute(...) != 0) {
    rollback_transaction();  // ✅ 插入 friend_requests 失败（比如网络超时），撤销一切
    return Error;
}

if (affected_rows != 1) {
    rollback_transaction();  // ✅ 用户不存在，撤销“假装插入”的假动作
    return NotFound;
}

if (!insert_event(...)) {
    rollback_transaction();  // ✅ 事件日志写失败了，撤销之前插入的请求记录
    return Error;
}

if (!commit_transaction(error)) {
    rollback_transaction();  // ✅ 提交指令发出去失败了，强制撤销
    return Error;
}
```

###### COMMIT

```c
//把所有已经执行成功的SQL修改，持久化到硬盘文件里，让数据尘埃落地
bool MySqlFriendRepository::commit_transaction(
    std::string& error
) {//服务起向MYSQL发送一各COMMIT指令包，0提交成功，非0失败
    if (mysql_commit(connection_) != 0) {
        error =
            "failed to commit transaction: " +
            std::string(mysql_error(connection_));
        return false;
    }

    return true;
}
```

 MySQL 服务器收到 `COMMIT` 后干了什么？（深入一步）

1.  **写入 Redo Log（重做日志）**：把事务修改的数据页变更，从内存日志缓冲区强制刷写到磁盘的 `ib_logfile` 文件中。这是 **WAL（Write-Ahead Logging）** 机制的核心——**先写日志，后写数据**。
2.  **标记事务为已提交**：在 `binlog`（二进制日志）和 Redo Log 中写入 `COMMIT` 标记，表示该事务已成功结束。
3.  **释放行锁（Row Locks）**：释放 `INSERT` 或 `SELECT ... FOR UPDATE` 持有的行锁和间隙锁，让其他事务（其他客户端）能够查询或修改这些数据行。
4.  **（异步）刷脏页**：InnoDB 后台线程会异步地把内存缓冲池（Buffer Pool）中对应的脏数据页，写入 `.ibd` 数据文件。**注意**：这一步通常是异步的，但有了步骤 1 的 Redo Log，即使数据库在数据页落盘前崩溃，重启后也能通过 Redo Log 恢复已提交的数据。

######  START TRANSACTION

MySQL 服务器收到 `START TRANSACTION` 后干了什么？

1.  **关闭自动提交（Autocommit）**：MySQL 默认每条 SQL 都自动提交（`autocommit=1`）。执行 `START TRANSACTION` 后，当前会话的自动提交被临时关闭，直到遇到 `COMMIT` 或 `ROLLBACK`。
2.  **分配事务 ID**：InnoDB 引擎为这个事务分配一个唯一的事务 ID（`TRX_ID`），用于 MVCC（多版本并发控制）和 Redo/Undo 日志跟踪。
3.  **记录 Undo Log 起点**：在 Undo 日志中标记一个“保存点”，以便后续 `ROLLBACK` 时能精确恢复到事务开始前的状态。
4.  **获取必要的锁**：根据后续 SQL 的执行情况，逐步获取行锁或表锁（但此时还没有锁定任何行，只是准备好了锁机制）。

为什么不能省略这一步？

如果直接执行 `INSERT` 而不调用 `begin_transaction`：

-   **默认自动提交**：每条 `INSERT` 执行后，MySQL 会立即将其持久化到硬盘。如果第二条 SQL（`insert_event`）失败，第一条 `INSERT INTO friend_requests` 已经写死了，无法撤销，导致**数据不一致**（有了请求记录但没有事件日志）。
-   **无法批量回滚**：没有事务包裹，`rollback` 就无法同时撤销两条 SQL 的修改。

而有了 `begin_transaction`，你才能用 `commit` 和 `rollback` 精确控制两条 SQL 的原子性。

###### PING

```c++
if (
        connection_ != nullptr &&
        mysql_ping(connection_) == 0//这个函数
    ) {
        return true;
    }
```

-   `mysql_ping(connection_) == 0`：`mysql_ping` 函数向服务器发送一个“心跳”包，如果服务器响应且连接正常，返回 0；如果连接已断开或服务器无响应，返回非 0。

##### 存储用户名

大多数系统会存 ID 以节省空间，但这里存用户名有一个巨大好处：**查询历史记录时，不需要 JOIN `users` 表去查名字，直接读出来就能用**，减少了联表查询的开销。代价是用户名如果改了，历史消息里的名字不会变（但用户名通常不允许修改，所以没问题）。

#### DAY18

##### observer模式

**Observer（观察者）模式**是软件设计中最重要的“行为型模式”之一，它的核心目的是**实现对象间的“一对多”依赖关系，当一个对象状态改变时，所有依赖它的对象都能自动收到通知并更新**

>   是一种单向的，一对多的状态同步机制

###### 模式的核心组件（基于你截图中的代码）

该模式由两个抽象角色和两种具体实现组成：

-   **抽象观察者（Observer）**：
    -   声明一个纯虚函数 `virtual void update() = 0;`。
    -   该函数作为通知入口点，由被观察者在状态变更时调用。
    
-   **具体观察者（Concrete Observer）**：
    -   继承自 `Observer`，实现 `update()` 方法。
    -   该方法内部定义针对被观察者状态变化的响应逻辑。
    
-   **抽象/具体被观察者（Observable / Subject）**：
    -   持有 **观察者抽象接口的指针容器**，即 `std::vector<Observer*> observers_`。
    
    -   提供注册接口 `register_(Observer* x)`，将观察者指针加入容器。
    
    -   提供注销接口 `unregister_(Observer* x)`，将观察者指针从容器移除。
    
    -   ##### 提供通知接口 `notifyObservers()`，该函数遍历容器，对每个元素执行 `x->update()`。







#### DAY19

##### 为什么main函数中经常用try-catch？

if-else：经常用在处理业务代码的时候，都是一些可预期的错误，轻量级

try-catch：用在main里，来处理预期外的错误。因为此时发生里系统不可预期的**重量级**错误，程序状态已经不可靠，就需要强制中断当前流程，向上抛catch



##### `(void)::signal(SIGPIPE, SIG_IGN);`

增加SIG_IGN信号，意在如果对方已经关闭了读端，那么操作系统会向进程发送一个SIGPIPE信号，会终止进程

增加这个信号之后，就不会直接杀死进程，而是返回错误码

前面的（void）就是说不接受这个有返回值的函数，强制转换成空函数

---



##### **One Loop Per Thread**

###### 1. 它解决了什么核心痛点？（Why）

在多线程网络编程中，如果多个线程同时操作一个 TCP 连接（比如线程A读，线程B写），就必然涉及**互斥锁（Mutex）**。锁会带来两个致命问题：

-   **上下文切换开销**：线程抢锁失败会被挂起，触发昂贵的系统调用。
-   **复杂的心智负担**：你第一章读到的“对象销毁竞态”、“死锁”，全都是多线程同时触碰同一份资源引起的。

**One Loop Per Thread 的策略是：**
**“一个 TCP 连接（`TcpConnection`）从出生到死亡，所有读写操作（包括上层业务回调），都必须固定由同一个线程（同一个 EventLoop）来执行。”**

这样就是串行执行，而不是并行执行，完美避开了竞态情况的出现

------

###### 2. 它的物理架构：主从 Reactor（Main-Reactor & Sub-Reactor）

在你那套完整代码中，`server_main.cpp` 和 `EventLoopThreadPool` 实现的就是标准的 **主从 Reactor** 模型：

-   **主 Reactor（MainReactor）**：
    -   只有一个线程（就是 `main` 函数所在的线程，`EventLoop mainLoop`）。
    -   它只负责一件事：`Acceptor` 监听倾听端口，接受新连接（`accept`）。
    -   收到新 socket 后，它绝不自己处理，而是把它**分发**给下面的 SubReactor。
-   **从 Reactor（SubReactor）**：
    -   通常有多个线程（比如 `workerThreads = 4`），每个线程拥有自己独立的 `EventLoop` 实例。
    -   它们负责所有已建立连接的 I/O 读写、定时器、以及业务回调。

**对应代码证据：**



```cpp
// server_main.cpp
TcpServer tcpServer(&mainLoop, listenAddress, "chat");
tcpServer.setThreadNum(workerThreads); // 设置 4 个 SubReactor 线程
```



```cpp
// TcpServer::newConnection (连接分发)
EventLoop* ioLoop = threadPool_->getNextLoop(); // 轮询（Round-Robin）选一个 SubReactor
TcpConnectionPtr conn = std::make_shared<TcpConnection>(ioLoop, ...); 
ioLoop->runInLoop([conn] { conn->connectEstablished(); }); // 把连接交给那个线程
```



------

###### 3. 内部机制：EventLoop 的死循环在做什么？

每个 SubReactor 线程都在执行 `EventLoop::loop()`，这是个永远不会退出的 `while` 循环。它的核心工作可以拆解为 3 步（对应 `EventLoop.cpp`）：

1.  **阻塞轮询（Poller）**：调用 `epoll_wait`（或 `poll`）挂在操作系统上。如果没有任何网络事件，该线程就在这里休眠，**不占用 CPU**。
2.  **事件分发（Channel）**：当 epoll 返回活跃的 socket 列表（`activeChannels_`）时，`loop` 会遍历这些 `Channel`，并调用 `channel->handleEvent()`。这一步会触发你注册的 `onMessage` 等回调。
3.  **执行跨线程任务（Pending Functors）**：这是 One Loop Per Thread 的“后门”。调用 `doPendingFunctors()` 执行其他线程交给它的任务（比如发送数据）。

------

###### 4. 如何实现“跨线程唤醒”？（最精彩的部分）

问题来了：如果 SubReactor 线程正阻塞在 `epoll_wait` 上，**别的线程（比如主线程）想让它发送数据，怎么叫醒它？**

答案就在 `EventLoop` 里的 **`wakeupFd_`（eventfd）**。

-   主线程调用 `conn->send()`，因为不在 I/O 线程，它会调用 `loop_->runInLoop()`。
-   `runInLoop` 把发送任务封装成 `Functor`，塞进该 SubReactor 的 `pendingFunctors_` 队列里。
-   随后，往该 SubReactor 的 `wakeupFd_` 写入一个字节（`EventLoop::wakeup()`）。
-   SubReactor 正阻塞在 `epoll_wait`，但 `wakeupFd_` 也是它监听的 Channel 之一。收到这个字节，`epoll_wait` 立即返回，线程被激活。
-   线程随后执行 `doPendingFunctors()`，把队列里的发送任务执行掉。

**这意味着什么？** 这套机制保证了：**所有的 I/O 操作（`sendInLoop`、`handleRead`）永远在同一个线程内部执行，根本不需要加锁！**

1.   首先是线程wait之前都会把一个特殊的wakeupfd加进去
2.   当主线程想向客户端发消息的时候，会把消息放进子线程的pendingfunctor里面
3.   此时虽然消息放进去了，但是线程仍然在沉睡，这个时候需要向wakeupfd写一个字节进去，这个时候子线程就会苏醒
4.   子线程苏醒后先把那个字节读取出来之后，再处理真正的peningfuctor,此时会向客户端发送消息



##### 前向声明

`class TcpConnection;`（注意后面是分号，没有花括号）。这行代码的作用是**提前告诉编译器：“有一个叫 `TcpConnection` 的类存在，你先记住这个名字，至于它里面有什么（成员变量和函数），我晚点再告诉你。”**

###### 如果没有前向声明，会发生什么灾难？（循环依赖）

假设你**不写** `class TcpConnection;`，而是直接 `#include "TcpConnection.h"`，会发生什么？

-   `TcpServer.h` 包含 `TcpConnection.h`。
-   `TcpConnection.h` 又需要包含 `TcpServer.h`（因为连接断开时，需要回调 `TcpServer` 的删除函数）。
-   结果：`TcpServer.h` -> `TcpConnection.h` -> `TcpServer.h` -> ... 无限循环。
-   编译器为了保护自己，会报错：`#include nested too deeply`（头文件嵌套过深）。

**前向声明解决了这个问题**：

-   在 `TcpServer.h` 中，我不需要知道 `TcpConnection` 里面有什么，我只需要存它的智能指针。所以我只写 `class TcpConnection;`。
-   在 `TcpServer.cpp`（实现文件）中，我再 `#include "TcpConnection.h"`，此时编译器才去查看 `TcpConnection` 的完整定义。

注意到文件开头的两行了吗？



```cpp
class Buffer;
class TcpConnection;
```

这是前向声明（你刚学的！）。这里没有 `#include "TcpConnection.h"`，因为如果包含了，`Callbacks.hpp` 就会依赖具体实现。而通过 `using TcpConnectionPtr = std::shared_ptr<TcpConnection>;`，编译器只需要知道 `TcpConnection` 是个合法的类名就够了，不需要知道它占多少内存。

#### DAY20

##### eventfd

###### 1. `eventfd` 是什么？（技术定义）

-   `eventfd` 是 Linux 内核提供的一种特殊文件描述符。
-   支持两种操作：
    -   **写入（`write`）**：向计数器中**增加值**（比如写入 `1`，计数器 +1）。
    -   **读取（`read`）**：读取计数器的值，并将其**重置为 0**（阻塞模式若无值可读会等待）。

------

###### 2. 在 `EventLoop` 中它怎么用？（解决什么问题）

在 Reactor 模型中，工作线程大部分时间阻塞在 `epoll_wait` 上。**如果主线程想把一个新客户端（`client_fd`）分配给该工作线程，必须让工作线程从 `epoll_wait` 中“醒”过来，并执行任务队列里的操作。**

**实现流程**：

1.  **初始化**：`EventLoop` 构造函数调用这个函数，获得一个 `wakeupFd_`。
2.  **注册**：`EventLoop` 将这个 `wakeupFd_` 封装成一个 `Channel`，注册到自己的 `epoll` 中，监听 `EPOLLIN` 事件。
3.  **跨线程唤醒（主线程 -> 工作线程）**：
    -   主线程调用 `workerLoop->queueInLoop(callback)`，把任务放入工作线程的任务队列。
    -   主线程立即执行 `::write(wakeupFd_, &one, sizeof(one))`（向 `eventfd` 写入 `1`）。
4.  **工作线程响应**：
    -   工作线程原本阻塞在 `epoll_wait`，现在 `epoll` 检测到 `wakeupFd_` 可读，立即返回。
    -   工作线程调用 `wakeupFd_` 对应的 `Channel::handleEvent()`，读取 `eventfd` 的值（清空计数器），然后执行任务队列里的所有回调





##### **`noexcept`**

承诺这个函数**绝对不会抛出任何异常**

#### DAY21

##### move

为什么在初始化列表中有一些变量是直接赋值的？但是有一些需要move转移所有权

这是因为有一些是指针，存的是内存地址，有一些是堆上的内存，比如说数组，推上的内存比较大所以需要move,只交换了几个指针和整数，没有堆内存分配，复杂度低



##### acceptor

```c++
acceptor_(std::make_unique<Acceptor>(loop, listenAddress)),
```

###### 创建指针

```c#
acceptor_ = std::make_unique<Acceptor>(loop, listenAddress);
```

| 优势           | 详细说明                                                     |
| :------------- | :----------------------------------------------------------- |
| **异常安全**   | 如果 `Acceptor` 构造函数抛出异常，`std::make_unique` 能保证**绝对不泄漏原始指针**。而 `new Acceptor` + `unique_ptr` 构造之间，若发生异常，可能导致内存泄漏。 |
| **简洁性**     | 不需要重复写类型名 `Acceptor`（只需写一次），避免了代码冗长和类型不匹配的错误。 |
| **性能微优化** | 在某些极端场景下，它避免了 `new` 和 `unique_ptr` 构造函数之间的临时指针存储开销（虽然现代编译器优化得很好，但语义上更干净）。 |

它就是把 `new` 出来的裸指针**立即托管**给 `unique_ptr`，确保从对象诞生的第一刻起，就有人负责它的生死。

###### 为什么是unique_ptr？

1.   acceptor代表一个监听套接字，进程中只能有一个acceptor负责监听端口，并且也不能复制（像sharedptr那样
2.   智能指针自动回收资源，RAll
3.   这个acceptor的生命周期完全由tcpserver管理，其他人无权管理



#### DAY22

##### cmake

###### 1. 基础环境配置（开头三板斧）



```cmake
cmake_minimum_required(VERSION 3.16)   # 锁定最低版本，防止老版本特性不支持
project(chatroom_v8_0 LANGUAGES CXX)   # 项目名，并声明只用 C++（加快解析）
```



-   **设置 C++ 标准**：

    

    ```cmake
    set(CMAKE_CXX_STANDARD 17)            # 使用 C++17
    set(CMAKE_CXX_STANDARD_REQUIRED ON)   # 如果编译器不支持 C++17 则报错
    set(CMAKE_CXX_EXTENSIONS OFF)         # 禁止使用 gcc/clang 特有扩展（保持跨平台）
    ```

    

------

###### 2. 查找外部依赖（找包）

CMake 不会自动识别你系统里的库，必须显式查找。

-   **找 MySQL（通过 pkg-config）**：

    

    ```cmake
    find_package(PkgConfig REQUIRED)          # 先启用 pkg-config 工具
    pkg_check_modules(MYSQL REQUIRED IMPORTED_TARGET mysqlclient)
    ```

    

    这里 `IMPORTED_TARGET` 会生成一个名为 `PkgConfig::MYSQL` 的 CMake 目标，后续直接链接它即可。

-   **找 OpenSSL 和 线程库**：

    

    ```cmake
    find_package(OpenSSL REQUIRED)   # 查找 OpenSSL（生成 OpenSSL::Crypto 等）
    find_package(Threads REQUIRED)   # 查找系统线程库（生成 Threads::Threads）
    ```

    

------

###### 3. 自定义函数（封装重复操作）

你定义了一个 `chat_warnings` 函数，用于给目标统一添加编译器警告标志，避免在每个 `add_executable` 后面重复写 `-Wall -Wextra`。



```cmakec
function(chat_warnings target)
    target_compile_options(
        ${target}
        PRIVATE
            -Wall -Wextra -Wpedantic   # PRIVATE 表示这些参数只影响该目标本身
    )
endfunction()
```



**调用方式**：`chat_warnings(minimuduo)` 即可。

------

###### 4. 创建库目标（静态库/动态库）

-   **创建静态库**：

    

    ```cmake
    add_library(minimuduo STATIC           # STATIC 生成 .a / .lib
        src/minimuduo/net/Buffer.cpp
        ...
    )
    ```

    

    **关键点**：源文件列表写在后面，若有多个目录，建议用 `GLOB`（不推荐）或用 `aux_source_directory`，但显式列出是 CMake 官方推荐做法（修改文件时 CMake 能自动感知）。

------

###### 5. 目标属性设置（核心：include 和 link）

这是现代 CMake 的精髓：**用 `target_\*` 命令，而不是 `include_directories` 全局设置**。

-   **设置头文件路径**：

    

    ```cmake
    target_include_directories(minimuduo PUBLIC include)
    ```

    

    -   `PUBLIC`：该路径不仅自己用，链接到此库的其他目标（如 `chat_core`）也会自动继承这个 `include` 路径。

-   **链接库**：

    

    ```cmake
    target_link_libraries(
        chat_core
        PUBLIC                         # 或 PRIVATE
            minimuduo
            PkgConfig::MYSQL
            OpenSSL::Crypto
            Threads::Threads
    )
    ```

    

    **可见性关键字（极其重要）**：

    -   `PRIVATE`：只在当前目标内部使用，链接者不继承。
    -   `PUBLIC`：当前目标使用，且**传递**给链接当前目标的上层目标。
    -   `INTERFACE`：当前目标本身不用，仅用于让链接者使用（常用于纯头文件库）。

------

###### 6. 生成可执行文件与测试

-   **生成可执行文件**：

    

    ```cmake
    add_executable(chat_server src/server_main.cpp)
    target_link_libraries(chat_server PRIVATE chat_core)  # server 依赖 core
    ```

    

    注意 `chat_client` 没有链接 `chat_core`，而是直接包含了 `src/protocol.cpp`，说明它是独立编译的。

-   **启用测试模块**：

    

    ```cmake
    enable_testing()   # 启用 CTest
    add_test(NAME proto_codec COMMAND test_proto)  # 注册测试
    ```

    

    运行测试时，进入 build 目录执行 `ctest`。

-   **设置测试属性（如超时）**：

    

    ```cmake
    set_tests_properties(
        master_sub_reactor_smoke
        PROPERTIES TIMEOUT 10   # 10 秒后自动终止，防止死循环卡住测试
    )
    ```

    

------

###### 7. 总结：现代 CMake 的黄金法则（针对你的脚本提炼）

| 原则                       | 你的脚本体现                                                 |
| :------------------------- | :----------------------------------------------------------- |
| **避免全局变量**           | 没有用 `link_directories` 或 `include_directories`，全部用 `target_` 前缀。 |
| **明确可见性**             | 指定了 `PUBLIC` / `PRIVATE`，让依赖传递变得可控。            |
| **尽量用 `IMPORTED` 目标** | 链接时写 `OpenSSL::Crypto` 而不是 `-lssl`，更跨平台。        |
| **将测试与主程序解耦**     | 测试目标（`test_xxx`）单独编译，并单独链接所需的最小依赖。   |

------



##### question

###### 1. 请简述本项目使用的网络模型，并说明主 Reactor 和子 Reactor 各自承担什么职责。

**答案**
采用 **主从 Reactor 多线程模型**（即 `EventLoop` + `EventLoopThreadPool`）。

-   **主 Reactor**（MainReactor）：运行在 `main` 线程，只负责监听 `Acceptor` 的 socket，接受新连接（`accept`），然后将新连接的 socket 分发给子 Reactor。
-   **子 Reactor**（SubReactor）：由 `EventLoopThreadPool` 管理多个工作线程，每个线程运行一个 `EventLoop`（事件循环），负责已建立连接的 I/O 事件处理（读、写、错误等）。
-   **优点**：主线程只处理 accept，负载轻；子线程并行处理 I/O，充分利用多核 CPU，提高并发能力。

------

###### 2. 什么是“粘包”问题？本项目如何解决？

**答案**
TCP 是流式协议，数据无边界，可能发生：

-   **粘包**：多个完整消息被合并到一起发送。
-   **拆包**：一个消息被分成多个 TCP 包发送。

本项目采用 **基于分隔符的文本行协议**（每条命令以 `\n` 结尾）。

-   在 `Buffer` 中通过 `findEOL()` 查找换行符；
-   若找到，则提取该行并消费，剩余数据保留在缓冲区；
-   若未找到，则继续等待后续数据；
-   若缓冲区积累超过 `kMaxInputBuffer`（8KB）仍未遇到换行符，则主动关闭连接，防止恶意超长包。

这种设计简单有效，且天然处理了粘包和拆包。

------

###### 3. Buffer 设计有何巧妙之处？如何避免频繁扩容和数据拷贝？

**答案**
`Buffer` 采用 `vector<char>` 作为动态缓冲区，并维护 `readerIndex_` 和 `writerIndex_` 两个指针（偏移量）。

-   **读操作**：移动 `readerIndex_`，不实际删除数据。
-   **写操作**：向 `writerIndex_` 位置写入，若可写空间不足，先尝试整理（将未读数据移到首部），若还不够则扩容。
-   **预置空间**：头部预留 8 字节（`kCheapPrepend`），方便将来扩展。
-   **减少拷贝**：`readFd` 使用 `readv` 分散读，利用栈上辅助缓冲区，避免反复扩容；`retrieve` 只是移动索引，不拷贝数据。

------

###### 4. 为什么使用 `eventfd` 而不是 `pipe` 作为唤醒机制？

**答案**
`eventfd` 是 Linux 提供的轻量级事件通知机制，比 `pipe` 更高效（只需一个文件描述符，内存占用小，读写操作简单）。本项目在 `EventLoop::wakeup()` 中写入一个 64 位整数，在 `handleWakeupRead()` 中读取，用于唤醒阻塞在 `epoll_wait` 中的线程，以便执行 `pendingFunctors`（跨线程任务）。



###### 5. 如何保证线程安全的“注册在线用户”操作？若同一用户重复登录会怎样？

**答案**
使用 `online_mutex_` 保护 `online_users_`（`unordered_map<string, weak_ptr<TcpConnection>>`）。

-   `register_online_user` 先加锁，检查该用户名是否已存在且连接有效，若存在则返回失败，防止重复登录。
-   当连接断开或用户主动注销时，`remove_online_user` 同样加锁移除映射。
-   `weak_ptr` 避免长期持有强引用导致连接无法释放，检查 `lock()` 是否为空来判断连接是否存活。

------

###### 6. 好友操作（添加/接受/拒绝）涉及多个数据库操作，如何保证原子性？为什么需要额外的 `friend_operation_mutex_`？

**答案**
数据库层面使用事务（`BEGIN` / `COMMIT` / `ROLLBACK`）保证多条 SQL 的原子性（如 `accept_friend_request` 先删请求再插好友关系）。
但服务器是多线程的，来自不同线程的请求可能并发修改同一用户的好友数据，因此 **除了数据库事务，还需要应用层的互斥锁**（`friend_operation_mutex_`）来串行化同一业务逻辑，防止出现竞争条件（例如两人同时互相加好友导致重复插入等）。该项目特意为好友和群组操作增加了独立的锁，因为子 Reactor 是多线程的。

------

###### 7. 跨线程调用（如 `send` 方法）是如何实现的？为什么要这样设计？

**答案**
`TcpConnection::send` 会检查当前线程是否属于该连接的 `EventLoop`，若是则直接调用 `sendInLoop`，否则通过 `loop_->runInLoop` 将发送操作投递到连接所属的 I/O 线程执行。
这样避免了多线程同时操作同一个 socket 描述符，保证了串行访问，且利用 `eventfd` 唤醒目标线程。同时，`runInLoop` 支持 `queueInLoop` 以应对可能的重入或大量回调。



###### 8. 如何存储用户密码？为什么选择 PBKDF2？

**答案**
存储格式为 `pbkdf2_sha256$iterations$salt$hash`，使用 PBKDF2 算法（迭代次数 210000，盐长 16 字节，哈希长 32 字节）。

-   PBKDF2 是密码学安全的密钥派生函数，能够抵抗暴力破解和彩虹表攻击。
-   盐值随机生成，使相同密码产生不同密文。
-   高迭代次数增加了计算成本，降低 GPU/ASIC 破解速度。
-   `CRYPTO_memcmp` 用于比较哈希，防止时序攻击。

------

###### 9. 私聊消息的离线存储是如何实现的？当用户上线时如何投递？

**答案**

-   发送私聊消息时，不仅插入 `messages` 表，还会插入一条记录到 `private_message_deliveries` 表，初始 `delivered_at_unix_ms` 为 `NULL`，表示未送达。
-   用户登录成功后，调用 `deliver_pending_messages` 查询该用户所有 `delivered_at_unix_ms IS NULL` 的记录，按顺序发送给客户端，每发送一条就更新为当前时间戳。
-   若用户在线，则立即发送并同步更新投递时间（见 `handle_private_message` 中的回调）。

**优点**：支持离线消息持久化，且投递状态清晰。

------

###### 10. 好友关系的存储为何使用 `user_low` 和 `user_high` 并保证 `user_low < user_high`？

**答案**
好友关系是无向的，如果存储两条记录（A-B 和 B-A），会导致数据冗余和查询复杂（需同时查两个方向）。
采用规范化（`user_low` 较小，`user_high` 较大）并加唯一约束，只需一条记录，查询时用 `normalize_pair` 生成固定顺序，配合索引高效检索。同时检查 `user_low < user_high` 避免自环。



###### 11. 为什么使用自定义的类似 Protobuf 的编码（varint + tag）而不是直接 JSON 或纯文本？

**答案**
自定义编码（参考 Protobuf）具有以下优势：

-   **紧凑**：采用 varint 编码整数，减少网络传输体积。
-   **灵活**：tag + wire 类型设计，支持字段可选、向后兼容（增加新字段不影响旧版本）。
-   **高效**：无需 JSON 的引号、花括号等冗余符号，解析速度快。

本项目将其用于 `friend_events`、`messages` 的 `payload` 列以及离线消息的序列化，保证存储和传输的紧凑性。

------

###### 12. 解析变长整型时，如何防止恶意数据导致死循环或内存耗尽？

**答案**

-   `read_varint` 函数限制最多循环 10 次（因为 varint 最大 64 位，每 7 位一组，最多 10 组），超过则返回失败，避免无限循环。
-   解析 `length-delimited` 字段时，会校验 `length` 是否超过输入剩余字节数，防止越界读取。
-   在 `skip_field` 中同样会检查长度，确保安全。

###### 13. `accept_friend_request` 方法中，先删除 `friend_requests` 再插入 `friendships`，如果插入失败会回滚，但为什么还要检查 `mysql_affected_rows`？

**答案**
首先，数据库事务能保证两条 SQL 要么同时成功要么同时失败（回滚）。但 `mysql_affected_rows` 用于确认是否真的删除了至少一行，从而判断请求是否存在。
若请求不存在（已被处理或从未存在），则应提前告知客户端“无此请求”，避免无谓的插入尝试。
即使出错，也会调用 `rollback()` 保证一致性。

------

###### 14. 群组管理员（Owner/Admin）的权限控制是如何实现的？如何防止管理员删除自己或 Owner？

**答案**

-   群组成员角色存储在 `group_members.member_role`（1=Owner，2=Admin，3=Member）。
-   在执行敏感操作（如 `remove_group_member`）时，先从数据库读取当前操作者的角色和目标用户的角色，然后根据规则判断：
    -   只有 Owner/Admin 能移除成员。
    -   Admin 不能移除 Owner 或另一个 Admin。
    -   Owner 可以移除任何成员（除自己外，Owner 只能解散群组）。
-   这些检查都在应用层加锁（`group_operation_mutex_`）并与数据库事务结合，保证原子性。



------

###### 16. 为什么离线消息投递采用“批量拉取 + 回调确认”方式，而不是逐条投递？

**答案**

-   减少数据库查询次数：一次性拉取最多 `kOfflineDeliveryBatch`（100 条），降低 I/O 压力。
-   避免长时间占用连接：发送每条消息时异步触发数据库更新，但使用 `send` 的完成回调来更新投递状态，保证顺序。
-   若批量拉取达到上限，提醒用户再次 `PENDING`，避免一次返回过多消息导致网络拥塞。



###### 17. 如何处理客户端异常断开（如突然断电）？

**答案**

-   服务端通过 `epoll` 检测到 `EPOLLHUP` 或 `read` 返回 0，会触发 `handleClose`，执行清理：从 `online_users_` 移除、广播下线通知、析构 `TcpConnection`。
-   使用 `shared_ptr` 管理连接，确保回调执行完毕后释放资源。
-   未完成的发送队列不会阻塞，连接关闭后直接丢弃。

------

###### 18. 密码哈希使用了 `PBKDF2`，但 `verify_password_pbkdf2` 中为何要限制迭代次数范围（10000～2000000）？

**答案**
防止恶意构造过小或过大的迭代次数，导致：

-   过小：弱安全性。
-   过大：消耗服务器 CPU，可能引发 DoS 攻击。
    通过范围校验确保哈希格式符合预期，同时也防止存储损坏的数据引起异常。



###### 19. 该项目如何支持将来扩展文件传输功能？

**答案**

-   预留了 `FileTransferMetadata` 和 `FileTransferStore` 接口，但未实现网络命令。
-   设计上可复用现有的 `TcpConnection` 和 `Buffer`，通过引入新的命令如 `FILE_SEND` 等，在 `handle_command` 中添加分支，并利用数据库或本地文件系统存储文件块。
-   同时可结合 Redis Pub/Sub 进行跨服务器通知。

##### `values.count`

`count` 成员函数用于**检查指定的键（Key）在容器中是否存在**。





