# Linux 编程学习笔记：进程间通信 IPC 与线程控制

> 目标：用通俗语言把 Linux 进程间通信和线程控制的核心概念讲清楚。  
> 适合：已经会一点 C 语言，正在学习 Linux 系统编程的人。  
> 建议阅读方式：先看“总览图”，再按章节学习；每个代码示例都建议自己敲一遍、编译、运行。

---

## 目录

- [0. 先建立整体认知](#0-先建立整体认知)
- [1. 第43章：进程间通信简介](#1-第43章进程间通信简介)
- [2. 第44章：管道和 FIFO](#2-第44章管道和-fifo)
- [3. 第29章：线程介绍](#3-第29章线程介绍)
- [4. 第30章：线程同步](#4-第30章线程同步)
- [5. 第31章：线程安全和每线程存储](#5-第31章线程安全和每线程存储)
- [6. 第32章：线程取消](#6-第32章线程取消)
- [7. 第33章：线程更多细节](#7-第33章线程更多细节)
- [8. 常见面试/考试题速记](#8-常见面试考试题速记)
- [9. 学习路线与练习建议](#9-学习路线与练习建议)

---

# 0. 先建立整体认知

## 0.1 进程、线程、IPC 到底是什么？

可以用一个很生活化的比喻来理解：

| 概念 | 通俗比喻 | Linux 中的含义 |
|---|---|---|
| 进程 process | 一个独立公司 | 一个正在运行的程序，有自己独立的地址空间、文件描述符表、信号处理方式等 |
| 线程 thread | 公司里的员工 | 同一个进程内部的执行流，共享进程资源，但每个线程有自己的栈、寄存器上下文 |
| IPC | 公司之间传文件、打电话、发消息 | 不同进程之间交换数据或同步动作 |
| 线程同步 | 同一公司多个员工抢同一台打印机，要排队 | 多个线程访问共享资源时，需要互斥量、条件变量等机制保证正确性 |

一句话总结：

> **IPC 解决“不同进程怎么交流”。线程同步解决“同一进程内多个线程怎么安全协作”。**

---

## 0.2 为什么进程之间不能直接访问彼此变量？

因为每个进程都有独立的虚拟地址空间。

例如：

```c
int x = 100;
```

进程 A 和进程 B 里都可能有变量 `x`，但它们的地址即使看起来相同，背后映射的物理内存也可能完全不同。

所以进程之间想通信，必须借助内核提供的机制，比如：

- 管道 pipe
- FIFO 命名管道
- socket
- 消息队列
- 共享内存
- 信号量
- 文件锁
- 信号 signal
- 内存映射 mmap

---

## 0.3 为什么线程之间通信更容易，但也更危险？

同一个进程里的线程共享地址空间：

```c
int counter = 0;
```

线程 A 和线程 B 都能直接访问 `counter`。

这很方便，但问题是：

```c
counter++;
```

这句看似一行，底层可能是三步：

1. 从内存读 counter 到寄存器
2. 寄存器加 1
3. 写回内存

如果两个线程同时执行，就可能发生数据竞争，最后结果错误。

所以线程编程的核心不是“怎么共享数据”，而是：

> **共享数据之后，如何保证它被正确地访问。**

---

## 0.4 IPC 和线程该怎么选？

| 场景 | 更适合 |
|---|---|
| 程序模块之间强隔离，崩一个不影响另一个 | 多进程 + IPC |
| 需要共享大量内存数据，高性能计算 | 多线程 |
| 客户端/服务器通信 | socket、FIFO、管道 |
| 父子进程之间传少量数据 | pipe |
| 无亲缘关系进程通信 | FIFO、socket、共享内存 |
| 极高吞吐量传输大量数据 | 共享内存 + 信号量/互斥机制 |
| 任务并发、共享缓存、线程池 | pthread |

---

## 0.5 编译示例程序

本笔记里的线程程序通常需要链接 pthread：

```bash
gcc demo.c -o demo -pthread
```

管道、FIFO、进程相关示例通常：

```bash
gcc demo.c -o demo
```

建议开启警告：

```bash
gcc demo.c -o demo -Wall -Wextra -O2 -pthread
```

---

# 1. 第43章：进程间通信简介

对应章节：

- 43.1 IPC 工具分类
- 43.2 通信工具
- 43.3 同步工具
- 43.4 IPC 工具比较
- 43.5 总结
- 43.6 习题

---

## 1.1 IPC 工具分类

Linux IPC 可以粗略分成两大类。

### 第一类：通信工具

通信工具负责“传数据”。

例如：

- 管道 pipe
- FIFO
- socket
- 消息队列
- 共享内存
- 文件
- mmap

问题是：

> A 进程的数据怎么到 B 进程那里？

---

### 第二类：同步工具

同步工具负责“协调时机”。

例如：

- 信号量 semaphore
- 文件锁
- 互斥量
- 条件变量
- futex
- 信号 signal

问题是：

> 谁先做？谁后做？什么时候可以读？什么时候可以写？

---

## 1.2 通信和同步有什么区别？

举个例子：餐厅后厨和前台。

- 后厨把菜做好，放到取餐口：这是**通信**
- 前台看到铃响才去取菜：这是**同步**

再举个进程间的例子：

- 进程 A 把数据写入共享内存：这是通信
- 进程 A 用信号量通知进程 B“数据准备好了”：这是同步

很多真实系统里，通信和同步会一起使用。

例如共享内存本身只负责共享数据，但它不告诉你什么时候能读、什么时候不能读，所以通常要搭配信号量或互斥锁。

---

## 1.3 常见 IPC 工具一览

| IPC 工具 | 能传数据吗 | 能同步吗 | 适合场景 |
|---|---:|---:|---|
| pipe 管道 | 是 | 部分可以 | 父子进程、命令行管道 |
| FIFO 命名管道 | 是 | 部分可以 | 无亲缘关系进程简单通信 |
| socket | 是 | 部分可以 | 本机或网络通信 |
| 消息队列 | 是 | 有一定同步效果 | 按消息为单位传输 |
| 共享内存 | 是，速度快 | 否，需要配合同步工具 | 大量数据共享 |
| 信号量 | 否 | 是 | 控制资源数量、进程同步 |
| 文件锁 | 否 | 是 | 多进程访问同一文件 |
| signal 信号 | 能传极少信息 | 是 | 通知事件发生 |
| mmap | 是 | 否，需要配合同步工具 | 文件映射、共享内存 |

---

## 1.4 几种 IPC 的直观理解

### 1.4.1 管道 pipe

像一根水管：

```text
进程 A 写入 ---> [ 管道 ] ---> 进程 B 读取
```

特点：

- 单向数据流
- 常用于父子进程
- 数据按字节流传输
- 没有名字，只能通过继承文件描述符使用

命令行里的：

```bash
ls | wc -l
```

就是典型管道。

`ls` 的标准输出连接到 `wc -l` 的标准输入。

---

### 1.4.2 FIFO 命名管道

FIFO 像“有名字的管道”。

普通 pipe 没有名字，只适合有亲缘关系的进程。FIFO 有路径名，例如：

```bash
/tmp/myfifo
```

无关进程只要知道这个路径，就能打开它通信。

---

### 1.4.3 socket

socket 像“电话”。

它可以：

- 同一台机器进程通信：Unix domain socket
- 不同机器网络通信：TCP/UDP socket

如果 pipe 是水管，socket 就是电话网络，适用范围更广。

---

### 1.4.4 消息队列

消息队列像“邮箱”。

每条消息是一个完整包裹：

```text
消息1：{类型=1, 内容="hello"}
消息2：{类型=2, 内容="status"}
```

接收方可以一条一条取。

优点：

- 保留消息边界
- 可以按消息类型读取
- 比字节流更结构化

---

### 1.4.5 共享内存

共享内存像“共享白板”。

多个进程把同一块物理内存映射进自己的地址空间：

```text
进程 A 地址空间  ----\
                    ---> 同一块物理内存
进程 B 地址空间  ----/
```

优点：

- 非常快
- 避免数据在内核和用户空间之间来回复制

缺点：

- 本身不提供同步
- 多个进程同时读写容易出错
- 常常需要搭配信号量、互斥量或 futex

---

### 1.4.6 信号量 semaphore

信号量像“停车场剩余车位牌”。

例如停车场有 3 个车位：

```text
sem = 3
```

来一辆车：

```text
sem--
```

走一辆车：

```text
sem++
```

如果 `sem == 0`，再来的车只能等待。

信号量常用于：

- 限制资源数量
- 生产者消费者模型
- 共享内存同步

---

### 1.4.7 signal 信号

signal 像“敲门”或“打断”。

例如：

- Ctrl+C 发送 SIGINT
- 子进程结束给父进程 SIGCHLD
- kill 命令发送信号

信号适合通知事件，但不适合传大量数据。

---

## 1.5 IPC 工具比较

| 工具 | 数据形式 | 是否有名字 | 速度 | 难度 | 典型用途 |
|---|---|---:|---:|---:|---|
| pipe | 字节流 | 否 | 中 | 低 | 父子进程通信 |
| FIFO | 字节流 | 是 | 中 | 低 | 无关进程简单通信 |
| socket | 字节流/数据报 | 是 | 中 | 中 | 网络/本机通信 |
| 消息队列 | 消息 | 是 | 中 | 中 | 结构化消息 |
| 共享内存 | 内存 | 是 | 高 | 高 | 大量数据共享 |
| 信号量 | 计数器 | 是 | 高 | 中 | 同步 |
| 信号 | 事件通知 | 否/进程号 | 高 | 中 | 异步通知 |

---

## 1.6 选择 IPC 的简单口诀

- **父子进程少量数据**：pipe
- **无关进程少量数据**：FIFO
- **跨机器通信**：socket
- **消息边界很重要**：消息队列
- **大量数据高性能**：共享内存
- **只想通知事件**：signal
- **需要控制访问顺序**：信号量/锁

---

# 2. 第44章：管道和 FIFO

对应章节：

- 44.1 概述
- 44.2 创建和使用管道
- 44.3 将管道作为一种进程同步的方法
- 44.4 使用管道连接过滤器
- 44.5 通过管道与 shell 命令进行通信：`popen()`
- 44.6 管道和 stdio 缓冲
- 44.7 FIFO
- 44.8 使用管道实现一个客户端/服务器应用程序
- 44.9 非阻塞 I/O
- 44.10 管道和 FIFO 中 `read()` 和 `write()` 的语义
- 44.11 总结

---

## 2.1 管道 pipe 概述

管道是最基础的 IPC 工具之一。

创建管道：

```c
int pipefd[2];
pipe(pipefd);
```

得到两个文件描述符：

```text
pipefd[0]：读端
pipefd[1]：写端
```

数据流向：

```text
write(pipefd[1])  --->  管道内核缓冲区  --->  read(pipefd[0])
```

注意：

> 管道是单向的。要双向通信，通常需要两个管道。

---

## 2.2 示例1：父进程给子进程发送消息

文件：`pipe_parent_child.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    int pipefd[2];
    pid_t pid;
    char buf[100];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // 子进程：只读，所以关闭写端
        close(pipefd[1]);

        ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
        if (n == -1) {
            perror("read");
            exit(1);
        }

        buf[n] = '\0';
        printf("Child received: %s\n", buf);

        close(pipefd[0]);
    } else {
        // 父进程：只写，所以关闭读端
        close(pipefd[0]);

        const char *msg = "Hello from parent";
        write(pipefd[1], msg, strlen(msg));

        close(pipefd[1]);
    }

    return 0;
}
```

编译运行：

```bash
gcc pipe_parent_child.c -o pipe_parent_child
./pipe_parent_child
```

可能输出：

```text
Child received: Hello from parent
```

关键点：

1. `pipe()` 必须在 `fork()` 之前调用，这样父子进程都能继承文件描述符。
2. 用不到的一端要关闭。
3. 读端读到 EOF 的条件是：所有写端都被关闭。

---

## 2.3 为什么要关闭不用的管道端？

假设子进程只读，但它没有关闭自己的写端。

那么即使父进程关闭了写端，内核仍然认为：

> 这个管道还有写端打开。

结果子进程 `read()` 可能一直阻塞，等不到 EOF。

所以常见模板是：

```text
父进程写：关闭读端
子进程读：关闭写端
```

---

## 2.4 管道作为同步工具

管道不仅能传数据，还能用于简单同步。

思路：

- 子进程完成初始化后，向管道写 1 个字节
- 父进程 `read()` 等待这个字节
- 读到了，说明子进程准备好了

### 示例2：用管道等待子进程准备好

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        close(pipefd[0]);

        printf("Child: doing initialization...\n");
        sleep(2);
        printf("Child: ready!\n");

        write(pipefd[1], "R", 1);
        close(pipefd[1]);
        exit(0);
    } else {
        char ch;
        close(pipefd[1]);

        printf("Parent: waiting for child...\n");
        read(pipefd[0], &ch, 1);
        printf("Parent: child is ready, continue.\n");

        close(pipefd[0]);
    }

    return 0;
}
```

这里管道的重点不是传输数据，而是让父进程“等一下”。

---

## 2.5 使用管道连接过滤器

Linux 命令行里很多工具是“过滤器”：

```text
输入 -> 处理 -> 输出
```

例如：

```bash
cat file.txt | grep hello | wc -l
```

这条命令里有两个管道：

```text
cat stdout -> grep stdin
grep stdout -> wc stdin
```

C 程序里可以通过：

- `pipe()`
- `fork()`
- `dup2()`
- `exec()`

实现类似效果。

---

## 2.6 示例3：C 程序实现 `ls | wc -l`

文件：`pipe_ls_wc.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(1);
    }

    if (pid1 == 0) {
        // 第一个子进程执行 ls
        close(pipefd[0]);

        // 把 stdout 重定向到管道写端
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execlp("ls", "ls", NULL);
        perror("execlp ls");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(1);
    }

    if (pid2 == 0) {
        // 第二个子进程执行 wc -l
        close(pipefd[1]);

        // 把 stdin 重定向到管道读端
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        execlp("wc", "wc", "-l", NULL);
        perror("execlp wc");
        exit(1);
    }

    // 父进程不读不写，必须关闭两端
    close(pipefd[0]);
    close(pipefd[1]);

    wait(NULL);
    wait(NULL);

    return 0;
}
```

重点理解 `dup2()`：

```c
dup2(pipefd[1], STDOUT_FILENO);
```

意思是：

> 以后写标准输出，其实就是写管道。

```c
dup2(pipefd[0], STDIN_FILENO);
```

意思是：

> 以后读标准输入，其实就是读管道。

---

## 2.7 `popen()`：和 shell 命令通信

`popen()` 是对 `pipe + fork + exec` 的封装。

函数原型：

```c
FILE *popen(const char *command, const char *type);
int pclose(FILE *stream);
```

`type` 可以是：

- `"r"`：从命令的标准输出读取
- `"w"`：写入命令的标准输入

---

### 示例4：读取 `ls` 命令输出

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *fp = popen("ls -1", "r");
    if (fp == NULL) {
        perror("popen");
        exit(1);
    }

    char line[256];

    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("Got: %s", line);
    }

    int status = pclose(fp);
    printf("Command exit status: %d\n", status);

    return 0;
}
```

通俗理解：

```text
你的程序 <--- pipe <--- shell command
```

注意：

- `popen()` 很方便，但会调用 shell。
- command 如果包含用户输入，要小心命令注入。
- `popen()` 默认是单向通信。

---

## 2.8 管道和 stdio 缓冲

`write()` 是系统调用，直接写文件描述符。

`printf()` / `fprintf()` 是 stdio 库函数，有用户态缓冲。

这会导致一个常见现象：

```c
printf("hello");
```

你以为马上输出了，其实可能还在缓冲区里。

缓冲类型：

| 类型 | 说明 |
|---|---|
| 无缓冲 | 立即输出，stderr 常见 |
| 行缓冲 | 遇到 `\n` 才刷新，终端常见 |
| 全缓冲 | 缓冲区满才刷新，文件/管道常见 |

所以在管道中使用 stdio 时，必要时调用：

```c
fflush(stdout);
```

或者让输出带换行。

---

## 2.9 FIFO 命名管道

FIFO 是有名字的管道。

创建方式：

```bash
mkfifo /tmp/myfifo
```

或者 C 语言：

```c
mkfifo("/tmp/myfifo", 0666);
```

使用方式和普通文件类似：

```c
open("/tmp/myfifo", O_RDONLY);
open("/tmp/myfifo", O_WRONLY);
```

但它背后不是磁盘文件，而是内核管道。

---

## 2.10 示例5：FIFO 写端

文件：`fifo_writer.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

int main(void) {
    const char *path = "/tmp/demo_fifo";

    if (mkfifo(path, 0666) == -1) {
        // 如果 FIFO 已经存在，可能会报错；这里简单忽略
        perror("mkfifo");
    }

    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("open writer");
        exit(1);
    }

    const char *msg = "Hello FIFO\n";
    write(fd, msg, strlen(msg));

    close(fd);
    return 0;
}
```

---

## 2.11 示例6：FIFO 读端

文件：`fifo_reader.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void) {
    const char *path = "/tmp/demo_fifo";

    if (mkfifo(path, 0666) == -1) {
        perror("mkfifo");
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open reader");
        exit(1);
    }

    char buf[100];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);

    if (n > 0) {
        buf[n] = '\0';
        printf("Reader got: %s", buf);
    }

    close(fd);
    return 0;
}
```

运行方式：

终端 1：

```bash
gcc fifo_reader.c -o fifo_reader
./fifo_reader
```

终端 2：

```bash
gcc fifo_writer.c -o fifo_writer
./fifo_writer
```

你会发现读端可能会阻塞等待写端。

---

## 2.12 FIFO 的阻塞行为

默认情况下：

| 操作 | 行为 |
|---|---|
| 只读打开 FIFO | 如果没有写端，会阻塞 |
| 只写打开 FIFO | 如果没有读端，会阻塞 |
| 读 FIFO | 没有数据但有写端，会阻塞 |
| 读 FIFO | 所有写端关闭，读到 EOF |
| 写 FIFO | 没有读端，可能触发 SIGPIPE 或返回 EPIPE |

可以使用 `O_NONBLOCK` 改成非阻塞。

---

## 2.13 非阻塞 I/O

打开文件时加上：

```c
O_NONBLOCK
```

例如：

```c
int fd = open("/tmp/demo_fifo", O_RDONLY | O_NONBLOCK);
```

非阻塞的意思是：

> 如果操作不能马上完成，不要睡眠等待，而是立刻返回错误。

常见错误：

```c
errno == EAGAIN
errno == EWOULDBLOCK
```

适合事件循环、服务器程序。

---

## 2.14 管道和 FIFO 中 read/write 的语义

### `read()` 行为

| 情况 | `read()` 结果 |
|---|---|
| 管道有数据 | 返回读到的字节数 |
| 管道没数据，但还有写端 | 阻塞 |
| 管道没数据，所有写端关闭 | 返回 0，表示 EOF |
| 非阻塞且无数据 | 返回 -1，errno 为 EAGAIN |

---

### `write()` 行为

| 情况 | `write()` 结果 |
|---|---|
| 有读端，管道有空间 | 写入成功 |
| 有读端，管道满 | 阻塞 |
| 没有读端 | 收到 SIGPIPE，或返回 -1/EPIPE |
| 非阻塞且管道满 | 返回 -1，errno 为 EAGAIN |

---

## 2.15 `PIPE_BUF` 和原子写

对于管道/FIFO：

> 当写入字节数不超过 `PIPE_BUF` 时，多个进程同时写入通常不会交叉混杂。

例如两个进程分别写：

```text
AAAA\n
BBBB\n
```

只要每次写入不超过 `PIPE_BUF`，读端不会读到：

```text
AABBA\n
```

但如果写入数据很大，可能被拆分，出现交错。

---

## 2.16 用 FIFO 实现简单客户端/服务器

思路：

- 服务器创建一个公共 FIFO，客户端把请求写进去
- 每个客户端自己创建一个私有 FIFO，用来接收响应
- 请求中带上客户端 FIFO 路径

```text
client ----请求----> public FIFO ----> server
client <---响应---- private FIFO <---- server
```

这个模型非常经典。

### 请求结构可以设计成：

```c
struct request {
    pid_t pid;
    int number;
};
```

客户端私有 FIFO：

```text
/tmp/client_fifo_12345
```

其中 `12345` 是客户端 PID。

服务器收到请求后，打开对应客户端 FIFO，把结果写回去。

---

## 2.17 管道/FIFO 总结

管道和 FIFO 的核心不是 API 多复杂，而是理解：

1. 管道有读端和写端。
2. 普通管道适合有亲缘关系进程。
3. FIFO 有路径名，适合无关进程。
4. 不用的一端必须关闭。
5. 阻塞行为非常重要。
6. EOF 的出现依赖所有写端是否关闭。
7. `dup2()` 可以把管道接到标准输入/输出，从而连接程序。

---

# 3. 第29章：线程介绍

对应章节：

- 29.1 概述
- 29.2 Pthreads API 的详细背景
- 29.3 创建线程
- 29.4 终止线程
- 29.5 线程 ID
- 29.6 连接已终止的线程
- 29.7 线程的分离
- 29.8 线程属性
- 29.9 线程 VS 进程
- 29.10 总结
- 29.11 练习

---

## 3.1 线程是什么？

线程是进程内部的一条执行路线。

一个进程至少有一个线程，也就是主线程。

多线程程序像这样：

```text
进程
├── 主线程
├── 工作线程 1
├── 工作线程 2
└── 工作线程 3
```

它们共享：

- 全局变量
- 堆内存
- 文件描述符
- 当前工作目录
- 信号处理设置

它们各自拥有：

- 线程 ID
- 栈
- 寄存器上下文
- errno
- 信号掩码
- 线程特有数据

---

## 3.2 为什么需要线程？

线程常用于：

1. 同时处理多个任务
2. 提高 CPU 利用率
3. 避免一个阻塞操作卡住整个程序
4. 服务器并发处理多个客户端
5. 后台任务，例如日志、定时刷新、计算

例如一个网络服务器：

```text
主线程：接受连接
工作线程1：处理客户端A
工作线程2：处理客户端B
工作线程3：处理客户端C
```

---

## 3.3 Pthreads API 背景

Linux 下常用 POSIX Threads，也就是 pthread。

常见函数：

| 函数 | 用途 |
|---|---|
| `pthread_create()` | 创建线程 |
| `pthread_exit()` | 终止当前线程 |
| `pthread_join()` | 等待线程结束并获取返回值 |
| `pthread_detach()` | 分离线程 |
| `pthread_self()` | 获取当前线程 ID |
| `pthread_equal()` | 比较两个线程 ID |
| `pthread_mutex_lock()` | 加锁 |
| `pthread_mutex_unlock()` | 解锁 |
| `pthread_cond_wait()` | 等待条件变量 |
| `pthread_cond_signal()` | 唤醒等待线程 |

编译时要加：

```bash
-pthread
```

不是简单的 `-lpthread`，因为 `-pthread` 还会设置编译器和链接器选项。

---

## 3.4 创建线程：`pthread_create()`

函数原型：

```c
int pthread_create(
    pthread_t *thread,
    const pthread_attr_t *attr,
    void *(*start)(void *),
    void *arg
);
```

通俗解释：

| 参数 | 说明 |
|---|---|
| `thread` | 用来保存新线程 ID |
| `attr` | 线程属性，通常传 `NULL` |
| `start` | 新线程要执行的函数 |
| `arg` | 传给线程函数的参数 |

线程函数格式必须是：

```c
void *thread_func(void *arg);
```

---

## 3.5 示例7：创建一个线程

文件：`thread_create.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *worker(void *arg) {
    char *msg = (char *)arg;
    printf("Worker thread says: %s\n", msg);
    return NULL;
}

int main(void) {
    pthread_t tid;

    int ret = pthread_create(&tid, NULL, worker, "hello");
    if (ret != 0) {
        fprintf(stderr, "pthread_create failed\n");
        exit(1);
    }

    pthread_join(tid, NULL);

    printf("Main thread done.\n");
    return 0;
}
```

编译：

```bash
gcc thread_create.c -o thread_create -pthread
./thread_create
```

理解流程：

```text
main 线程
  |
  | pthread_create()
  v
worker 线程开始运行

main 线程 pthread_join() 等待 worker 结束
```

---

## 3.6 线程终止方式

线程可以通过以下方式结束：

1. 线程函数 `return`
2. 调用 `pthread_exit()`
3. 被其他线程取消
4. 整个进程调用 `exit()` 或主函数返回，导致所有线程结束

注意：

> `return` 只结束当前线程函数；`exit()` 会结束整个进程里的所有线程。

---

## 3.7 `pthread_exit()`

```c
void pthread_exit(void *retval);
```

它用于结束当前线程，并可返回一个值给 `pthread_join()`。

### 示例8：线程返回计算结果

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *calc(void *arg) {
    int n = *(int *)arg;

    int *result = malloc(sizeof(int));
    if (result == NULL) {
        pthread_exit(NULL);
    }

    *result = n * n;
    pthread_exit(result);
}

int main(void) {
    pthread_t tid;
    int value = 7;

    pthread_create(&tid, NULL, calc, &value);

    void *ret;
    pthread_join(tid, &ret);

    int *answer = (int *)ret;
    if (answer != NULL) {
        printf("Result = %d\n", *answer);
        free(answer);
    }

    return 0;
}
```

注意：

不要返回局部变量地址：

```c
void *bad(void *arg) {
    int x = 100;
    return &x;   // 错误：线程函数返回后，栈变量失效
}
```

可以返回：

- `malloc()` 分配的内存
- 全局变量地址
- 传入参数中有效对象的地址

---

## 3.8 线程 ID：`pthread_t`

获取当前线程 ID：

```c
pthread_t tid = pthread_self();
```

比较线程 ID：

```c
pthread_equal(tid1, tid2)
```

不要假设 `pthread_t` 一定是整数。有的系统里它可能是结构体或指针类型。

为了打印，在 Linux/glibc 下常见写法是：

```c
printf("%lu\n", (unsigned long)pthread_self());
```

但跨平台程序最好不要依赖它的具体类型。

---

## 3.9 `pthread_join()`：连接已终止线程

```c
int pthread_join(pthread_t thread, void **retval);
```

作用：

1. 等待指定线程结束
2. 获取线程返回值
3. 回收线程资源

如果创建了一个可连接线程，但从不 `join`，它结束后仍会保留一部分资源，类似“僵尸线程”。

所以口诀：

> 创建普通线程后，要么 `pthread_join()`，要么 `pthread_detach()`。

---

## 3.10 线程分离：`pthread_detach()`

分离线程的意思是：

> 线程结束后自动释放资源，不需要别人 join。

```c
pthread_detach(tid);
```

适合“后台任务”：

```c
void *background(void *arg) {
    // 做完就退出，不需要返回结果
    return NULL;
}
```

注意：

- 分离线程不能再被 `pthread_join()`
- 如果需要拿返回值，就不要 detach
- 如果不关心返回值，可以 detach

---

## 3.11 示例9：分离线程

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void *task(void *arg) {
    sleep(1);
    printf("Detached thread finished.\n");
    return NULL;
}

int main(void) {
    pthread_t tid;

    pthread_create(&tid, NULL, task, NULL);
    pthread_detach(tid);

    printf("Main continues...\n");
    sleep(2);

    return 0;
}
```

如果 `main` 很快结束，整个进程结束，分离线程也会被强制结束。所以这里 `sleep(2)` 是为了让你看到输出。

---

## 3.12 线程属性

`pthread_attr_t` 可以设置线程属性，例如：

- 栈大小
- 是否分离
- 调度策略
- 调度优先级
- 栈地址

常见用法：

```c
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
pthread_create(&tid, &attr, worker, NULL);
pthread_attr_destroy(&attr);
```

大多数入门场景传 `NULL` 就够了。

---

## 3.13 线程 VS 进程

| 对比 | 进程 | 线程 |
|---|---|---|
| 地址空间 | 独立 | 同一进程内共享 |
| 创建开销 | 较大 | 较小 |
| 通信 | 需要 IPC | 直接共享变量 |
| 崩溃影响 | 通常影响自身 | 一个线程崩溃可能导致整个进程崩溃 |
| 安全隔离 | 好 | 弱 |
| 调试难度 | 中 | 较高 |
| 适用场景 | 强隔离、多服务 | 高并发、共享数据 |

简单判断：

- 想要隔离：用进程
- 想要轻量共享：用线程

---

# 4. 第30章：线程同步

对应章节：

- 30.1 保护对共享变量的访问：互斥量
- 30.1.1 静态分配的互斥量
- 30.1.2 加锁和解锁互斥量
- 30.1.3 互斥量的性能
- 30.1.4 互斥量的死锁
- 30.1.5 动态初始化互斥量
- 30.1.6 互斥量的属性
- 30.1.7 互斥量类型
- 30.2 通知状态的改变：条件变量
- 30.2.1 静态分配的条件变量
- 30.2.2 通知和等待条件变量
- 30.2.3 测试条件变量的判断条件 predicate
- 30.2.4 示例程序：连接任意已终止线程
- 30.2.5 动态分配的条件变量
- 30.3 总结
- 30.4 练习

---

## 4.1 为什么需要同步？

看下面代码：

```c
counter++;
```

如果两个线程各执行 100000 次，理论上结果应该是 200000。

但如果不加锁，可能小于 200000。

原因是：

```text
线程 A 读 counter = 10
线程 B 读 counter = 10
线程 A 写 counter = 11
线程 B 写 counter = 11
```

两个加一操作最后只加了一次。

这叫：

> 数据竞争 race condition

---

## 4.2 互斥量 mutex

互斥量像厕所门锁。

- 进去前先锁门
- 出来后解锁
- 门锁着时，别人只能等

使用步骤：

```c
pthread_mutex_lock(&mutex);
// 访问共享资源
pthread_mutex_unlock(&mutex);
```

被保护的区域叫：

> 临界区 critical section

---

## 4.3 静态分配互斥量

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
```

适合全局变量或静态变量。

---

## 4.4 示例10：不加锁导致计数错误

文件：`race_counter.c`

```c
#include <stdio.h>
#include <pthread.h>

#define LOOPS 1000000

long counter = 0;

void *worker(void *arg) {
    for (int i = 0; i < LOOPS; i++) {
        counter++;
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("counter = %ld, expected = %d\n", counter, LOOPS * 2);
    return 0;
}
```

多运行几次，你可能看到结果不稳定。

---

## 4.5 示例11：使用互斥量修复计数错误

文件：`mutex_counter.c`

```c
#include <stdio.h>
#include <pthread.h>

#define LOOPS 1000000

long counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg) {
    for (int i = 0; i < LOOPS; i++) {
        pthread_mutex_lock(&mutex);
        counter++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("counter = %ld, expected = %d\n", counter, LOOPS * 2);
    return 0;
}
```

这次结果应该稳定正确。

但注意：

> 锁不是越多越好。锁会带来性能开销，并可能造成死锁。

---

## 4.6 互斥量性能

加锁有成本：

1. 无竞争时，成本较低
2. 有竞争时，线程可能阻塞、切换上下文，成本变高
3. 临界区越大，其他线程等待越久

优化原则：

- 临界区尽量短
- 不要在持锁期间做耗时操作
- 不要在持锁期间执行可能阻塞的 I/O
- 能用局部变量先算完，再一次性更新共享变量

例如：

```c
// 不推荐
pthread_mutex_lock(&mutex);
do_slow_io();
counter++;
pthread_mutex_unlock(&mutex);

// 更好
do_slow_io();
pthread_mutex_lock(&mutex);
counter++;
pthread_mutex_unlock(&mutex);
```

---

## 4.7 死锁 deadlock

死锁是：

> 多个线程互相等待对方释放资源，结果谁也走不了。

经典场景：

```text
线程 A 拿到锁 1，等待锁 2
线程 B 拿到锁 2，等待锁 1
```

代码示意：

```c
// 线程 A
pthread_mutex_lock(&m1);
pthread_mutex_lock(&m2);

// 线程 B
pthread_mutex_lock(&m2);
pthread_mutex_lock(&m1);
```

如果 A 拿了 m1，B 拿了 m2，就互相卡死。

---

## 4.8 避免死锁的方法

1. 固定加锁顺序  
   所有线程都先锁 m1，再锁 m2。

2. 减少同时持有多把锁  
   能一把锁解决就不要多把。

3. 使用 `pthread_mutex_trylock()`  
   拿不到锁就先释放已有锁，稍后重试。

4. 不要持锁调用未知代码  
   比如回调函数、复杂库函数。

5. 保持临界区短小。

---

## 4.9 动态初始化互斥量

如果互斥量在堆上，或者需要设置属性，可以用：

```c
pthread_mutex_t mutex;
pthread_mutex_init(&mutex, NULL);
...
pthread_mutex_destroy(&mutex);
```

示例：

```c
pthread_mutex_t *m = malloc(sizeof(pthread_mutex_t));
pthread_mutex_init(m, NULL);

pthread_mutex_lock(m);
// ...
pthread_mutex_unlock(m);

pthread_mutex_destroy(m);
free(m);
```

---

## 4.10 互斥量类型

常见类型：

| 类型 | 行为 |
|---|---|
| 普通 mutex | 默认，重复加锁行为未定义或死锁 |
| 错误检查 mutex | 重复加锁会返回错误 |
| 递归 mutex | 同一线程可以多次加锁，需要对应次数解锁 |
| 自适应 mutex | 实现相关，可能先自旋再阻塞 |

初学建议：

> 默认 mutex 足够。递归锁不要滥用，它可能掩盖设计问题。

---

## 4.11 条件变量 condition variable

互斥量解决的是：

> 同一时间只允许一个线程访问共享资源。

条件变量解决的是：

> 某个条件不满足时，让线程睡眠等待；条件满足时，唤醒它。

典型例子：生产者消费者。

```text
队列为空：
  消费者不能取，应该睡觉等待

生产者放入数据：
  通知消费者醒来
```

条件变量总是和互斥量一起使用。

---

## 4.12 静态分配条件变量

```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
```

---

## 4.13 等待和通知条件变量

等待：

```c
pthread_mutex_lock(&mutex);

while (条件不满足) {
    pthread_cond_wait(&cond, &mutex);
}

处理共享数据;

pthread_mutex_unlock(&mutex);
```

通知：

```c
pthread_mutex_lock(&mutex);

修改共享状态，让条件满足;

pthread_cond_signal(&cond);
// 或 pthread_cond_broadcast(&cond);

pthread_mutex_unlock(&mutex);
```

---

## 4.14 `pthread_cond_wait()` 的关键点

```c
pthread_cond_wait(&cond, &mutex);
```

它做了三件事：

1. 原子地释放 mutex
2. 让当前线程睡眠等待 cond
3. 被唤醒后，重新加锁 mutex，再返回

为什么要释放 mutex？

因为如果消费者拿着锁睡觉，生产者就没法获得锁放数据，程序会卡死。

---

## 4.15 为什么要用 while，而不是 if？

错误写法：

```c
if (queue_empty()) {
    pthread_cond_wait(&cond, &mutex);
}
```

正确写法：

```c
while (queue_empty()) {
    pthread_cond_wait(&cond, &mutex);
}
```

原因：

1. 可能出现虚假唤醒 spurious wakeup
2. 可能多个消费者被唤醒，但只有一个拿到数据
3. 被唤醒只表示“条件可能变了”，不保证条件一定满足

所以口诀：

> 等条件变量，一定用 while 检查 predicate。

predicate 就是“判断条件”，比如：

```c
count > 0
```

---

## 4.16 示例12：生产者消费者模型

文件：`producer_consumer.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define ITEMS 20

int buffer[BUFFER_SIZE];
int count = 0;
int in = 0;
int out = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

void *producer(void *arg) {
    for (int i = 1; i <= ITEMS; i++) {
        pthread_mutex_lock(&mutex);

        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&not_full, &mutex);
        }

        buffer[in] = i;
        in = (in + 1) % BUFFER_SIZE;
        count++;

        printf("Produced %d, count=%d\n", i, count);

        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mutex);

        usleep(100000);
    }

    return NULL;
}

void *consumer(void *arg) {
    for (int i = 1; i <= ITEMS; i++) {
        pthread_mutex_lock(&mutex);

        while (count == 0) {
            pthread_cond_wait(&not_empty, &mutex);
        }

        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;

        printf("Consumed %d, count=%d\n", item, count);

        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mutex);

        usleep(150000);
    }

    return NULL;
}

int main(void) {
    pthread_t p, c;

    pthread_create(&p, NULL, producer, NULL);
    pthread_create(&c, NULL, consumer, NULL);

    pthread_join(p, NULL);
    pthread_join(c, NULL);

    return 0;
}
```

理解这个例子：

- `mutex` 保护共享队列
- `not_empty` 表示“队列非空，可以消费”
- `not_full` 表示“队列未满，可以生产”
- 消费者发现队列空，就等待 `not_empty`
- 生产者放入数据后，通知 `not_empty`
- 生产者发现队列满，就等待 `not_full`
- 消费者取走数据后，通知 `not_full`

---

## 4.17 `pthread_cond_signal()` 和 `pthread_cond_broadcast()`

| 函数 | 作用 |
|---|---|
| `pthread_cond_signal()` | 唤醒至少一个等待线程 |
| `pthread_cond_broadcast()` | 唤醒所有等待线程 |

什么时候用 signal？

- 只有一个线程需要处理变化
- 例如队列里新增一个任务

什么时候用 broadcast？

- 状态变化可能影响所有等待者
- 例如配置加载完成、程序准备退出、资源整体状态改变

---

## 4.18 示例：连接任意已终止线程的思想

普通 `pthread_join(tid)` 必须指定某个线程。

如果你想实现：

> 哪个线程先结束，就先处理哪个线程。

可以让工作线程结束前：

1. 加锁
2. 把自己的 ID 或结果放入“完成队列”
3. 修改完成计数
4. `pthread_cond_signal()` 通知主线程

主线程：

1. 等待完成队列非空
2. 取出一个已完成线程信息
3. 再进行 join 或处理结果

简化伪代码：

```c
worker:
    do_work();
    lock();
    finished_count++;
    put_self_into_finished_list();
    signal(cond);
    unlock();

main:
    lock();
    while (finished_count == 0)
        wait(cond, mutex);
    get_finished_thread();
    unlock();
```

这个例子说明条件变量的本质：

> 它不是传数据的工具，而是通知“某个状态变了”。

真正的状态要放在共享变量里，比如 `finished_count`。

---

## 4.19 线程同步总结

- 共享变量必须受保护。
- 互斥量保护“临界区”。
- 条件变量用于“等待某个条件成立”。
- `pthread_cond_wait()` 必须和 mutex 搭配。
- 等条件变量一定用 `while`。
- 小心死锁。
- 不要持锁做慢操作。

---

# 5. 第31章：线程安全和每线程存储

对应章节：

- 31.1 线程安全，再论可重入性
- 31.2 一次性初始化
- 31.3 线程特有数据
- 31.3.1 库函数视角下的线程特有数据
- 31.3.2 线程特有数据 API 概述
- 31.3.3 线程特有数据 API 详述
- 31.3.4 使用线程特有数据 API
- 31.3.5 线程特有数据的实现限制
- 31.4 线程局部存储
- 31.5 总结
- 31.6 练习

---

## 5.1 什么是线程安全？

一个函数如果能被多个线程同时调用，并且结果仍然正确，就叫线程安全。

例如：

```c
int add(int a, int b) {
    return a + b;
}
```

它没有共享状态，通常是线程安全的。

但下面这个函数就有问题：

```c
char *bad_format(int n) {
    static char buf[100];
    snprintf(buf, sizeof(buf), "%d", n);
    return buf;
}
```

因为 `buf` 是静态共享的。两个线程同时调用，会互相覆盖。

---

## 5.2 可重入函数 reentrant

可重入比线程安全更严格。

一个函数可重入，通常意味着：

- 不使用静态可修改数据
- 不返回静态缓冲区
- 不调用不可重入函数
- 不依赖全局状态
- 可以在信号处理函数中安全再次进入

通俗理解：

> 线程安全是“多线程同时用没事”。  
> 可重入是“执行到一半被打断，再进入一次也没事”。

---

## 5.3 常见导致线程不安全的原因

1. 使用全局变量
2. 使用静态局部变量
3. 返回静态缓冲区
4. 使用共享资源但不加锁
5. 调用非线程安全库函数
6. 使用 `errno` 这种看似全局但实际每线程一份的机制时理解错误

---

## 5.4 让函数线程安全的方法

| 方法 | 思路 |
|---|---|
| 不共享数据 | 使用局部变量 |
| 调用者提供缓冲区 | 避免内部 static buffer |
| 加锁 | 保护共享状态 |
| 每线程存储 | 每个线程一份数据 |
| 线程局部存储 | 使用 `__thread` 或 `_Thread_local` |

---

## 5.5 示例13：把不安全函数改成安全函数

不安全版本：

```c
char *format_number(int n) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "number=%d", n);
    return buf;
}
```

安全版本：让调用者提供缓冲区。

```c
int format_number_r(int n, char *buf, size_t size) {
    return snprintf(buf, size, "number=%d", n);
}
```

很多库函数的 `_r` 版本就是这个思想。

例如：

```c
strtok()      // 不安全或不适合多线程
strtok_r()    // 更安全，状态由调用者传入
```

---

## 5.6 一次性初始化：`pthread_once()`

有些初始化只应该执行一次，比如：

- 初始化全局锁
- 初始化配置
- 创建线程特有数据 key
- 初始化某个库

如果多个线程同时第一次调用，可能重复初始化。

`pthread_once()` 可以保证初始化函数只执行一次。

```c
pthread_once_t once = PTHREAD_ONCE_INIT;

void init_func(void) {
    // 只执行一次
}

pthread_once(&once, init_func);
```

---

## 5.7 示例14：`pthread_once()`

```c
#include <stdio.h>
#include <pthread.h>

pthread_once_t once = PTHREAD_ONCE_INIT;

void init_library(void) {
    printf("Library initialized once.\n");
}

void *worker(void *arg) {
    pthread_once(&once, init_library);
    printf("Thread working.\n");
    return NULL;
}

int main(void) {
    pthread_t t1, t2, t3;

    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);
    pthread_create(&t3, NULL, worker, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    return 0;
}
```

无论几个线程调用，`init_library()` 只会执行一次。

---

## 5.8 线程特有数据 TSD

线程特有数据 Thread-Specific Data，简称 TSD。

它的目的：

> 同一个 key，每个线程取到的是自己的 value。

像酒店前台的房卡系统：

```text
key = 房间号类型
线程 A -> 自己的数据
线程 B -> 自己的数据
线程 C -> 自己的数据
```

常见 API：

| 函数 | 作用 |
|---|---|
| `pthread_key_create()` | 创建 key |
| `pthread_setspecific()` | 设置当前线程对应 key 的值 |
| `pthread_getspecific()` | 获取当前线程对应 key 的值 |
| `pthread_key_delete()` | 删除 key |

---

## 5.9 TSD 的典型用途

假设你写一个库函数：

```c
char *get_error_string(void);
```

你想返回一个字符串，但不能用全局静态 buffer，因为多线程会互相覆盖。

可以给每个线程分配自己的 buffer：

```text
线程 A 调用 -> 返回 A 自己的 buffer
线程 B 调用 -> 返回 B 自己的 buffer
```

这样函数接口仍然简单，同时避免线程互相覆盖。

---

## 5.10 示例15：使用线程特有数据

文件：`tsd_demo.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_key_t key;
pthread_once_t once = PTHREAD_ONCE_INIT;

void destructor(void *ptr) {
    free(ptr);
}

void create_key(void) {
    pthread_key_create(&key, destructor);
}

void *worker(void *arg) {
    pthread_once(&once, create_key);

    int *value = malloc(sizeof(int));
    *value = (int)(long)arg;

    pthread_setspecific(key, value);

    int *my_value = pthread_getspecific(key);
    printf("Thread %ld has value %d\n", (long)arg, *my_value);

    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, worker, (void *)1);
    pthread_create(&t2, NULL, worker, (void *)2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_key_delete(key);
    return 0;
}
```

这里：

- `key` 是全局的
- 每个线程通过同一个 `key` 存入自己的 `value`
- 线程结束时，destructor 自动释放对应数据

---

## 5.11 TSD 的实现限制

系统通常会限制：

- 每个进程最多能创建多少 key
- 每个线程能保存多少特有数据
- destructor 可能被多轮调用

实际写程序时：

- key 不要随便大量创建
- 通常一个模块创建少量 key
- 使用 `pthread_once()` 保证 key 只创建一次

---

## 5.12 线程局部存储 TLS

线程局部存储 Thread-Local Storage，简称 TLS。

C11 写法：

```c
_Thread_local int x;
```

GCC 常见写法：

```c
__thread int x;
```

意思是：

> 看起来是全局变量，但每个线程都有自己的一份。

---

## 5.13 示例16：线程局部变量

```c
#include <stdio.h>
#include <pthread.h>

__thread int tls_counter = 0;

void *worker(void *arg) {
    for (int i = 0; i < 3; i++) {
        tls_counter++;
        printf("Thread %ld: tls_counter=%d\n", (long)arg, tls_counter);
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, worker, (void *)1);
    pthread_create(&t2, NULL, worker, (void *)2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
```

两个线程看到的是各自的 `tls_counter`，互不影响。

---

## 5.14 TSD 和 TLS 怎么选？

| 对比 | TSD | TLS |
|---|---|---|
| 使用方式 | API 操作 key/value | 像普通变量 |
| 灵活性 | 高，可以动态分配 | 较低 |
| 性能 | 略有开销 | 通常更快 |
| 析构函数 | 支持 destructor | 取决于实现和语言 |
| 适合 | 库设计、动态数据 | 简单每线程变量 |

简单口诀：

- 普通程序里每线程变量：TLS
- 写库、需要 destructor 或动态管理：TSD

---

# 6. 第32章：线程取消

对应章节：

- 32.1 取消一个线程
- 32.2 取消状态及类型
- 32.3 取消点
- 32.4 线程可取消性的检测
- 32.5 清理函数 cleanup handler
- 32.6 异步取消
- 32.7 总结

---

## 6.1 什么是线程取消？

线程取消就是一个线程请求另一个线程结束。

```c
pthread_cancel(tid);
```

注意：

> `pthread_cancel()` 不是强制立即杀死线程。默认情况下，它只是发送取消请求。

被取消线程是否结束，取决于它的取消状态和取消类型。

---

## 6.2 取消状态

线程可以设置自己是否允许被取消。

```c
pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
```

| 状态 | 含义 |
|---|---|
| `PTHREAD_CANCEL_ENABLE` | 允许取消，默认 |
| `PTHREAD_CANCEL_DISABLE` | 暂时不允许取消 |

适合保护关键区域：

```c
pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);

// 不能被取消的关键操作

pthread_setcancelstate(old, NULL);
```

---

## 6.3 取消类型

```c
pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
```

| 类型 | 含义 |
|---|---|
| 延迟取消 deferred | 只在取消点响应，默认 |
| 异步取消 asynchronous | 几乎任何时候都可能被取消 |

建议：

> 尽量使用默认的延迟取消，不要轻易使用异步取消。

因为异步取消可能在持锁、分配内存、修改数据结构时突然终止线程，导致资源泄漏或状态损坏。

---

## 6.4 取消点 cancellation point

默认延迟取消下，线程只有到达取消点才会真正退出。

常见取消点包括：

- `read()`
- `write()` 在某些情况下
- `sleep()`
- `pthread_cond_wait()`
- `pthread_join()`
- `accept()`
- `select()`
- `poll()`

也可以手动设置取消点：

```c
pthread_testcancel();
```

---

## 6.5 示例17：取消一个线程

文件：`cancel_demo.c`

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *worker(void *arg) {
    while (1) {
        printf("Worker running...\n");
        sleep(1);  // sleep 是取消点
    }
    return NULL;
}

int main(void) {
    pthread_t tid;

    pthread_create(&tid, NULL, worker, NULL);

    sleep(3);
    printf("Main: cancel worker\n");
    pthread_cancel(tid);

    void *ret;
    pthread_join(tid, &ret);

    if (ret == PTHREAD_CANCELED) {
        printf("Worker was canceled.\n");
    }

    return 0;
}
```

---

## 6.6 清理函数 cleanup handler

线程被取消时，可能需要释放资源：

- 解锁 mutex
- 释放 malloc 内存
- 关闭文件描述符
- 删除临时文件

pthread 提供：

```c
pthread_cleanup_push(cleanup_func, arg);
pthread_cleanup_pop(execute);
```

注意：这两个通常是宏，必须成对出现在同一作用域里。

---

## 6.7 示例18：取消时自动解锁

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void cleanup_unlock(void *arg) {
    pthread_mutex_unlock((pthread_mutex_t *)arg);
    printf("Cleanup: mutex unlocked.\n");
}

void *worker(void *arg) {
    pthread_mutex_lock(&mutex);
    pthread_cleanup_push(cleanup_unlock, &mutex);

    printf("Worker: mutex locked, now sleeping...\n");
    sleep(10);  // 取消点

    pthread_cleanup_pop(1);
    return NULL;
}

int main(void) {
    pthread_t tid;

    pthread_create(&tid, NULL, worker, NULL);

    sleep(2);
    pthread_cancel(tid);
    pthread_join(tid, NULL);

    printf("Main: trying to lock mutex...\n");
    pthread_mutex_lock(&mutex);
    printf("Main: got mutex, no deadlock.\n");
    pthread_mutex_unlock(&mutex);

    return 0;
}
```

如果没有 cleanup handler，线程在持锁期间被取消，mutex 可能永远不解锁，导致其他线程死锁。

---

## 6.8 异步取消为什么危险？

假设线程正在执行：

```c
pthread_mutex_lock(&mutex);
shared_list_add(node);
pthread_mutex_unlock(&mutex);
```

如果异步取消发生在 `shared_list_add()` 中间：

- mutex 没释放
- 链表可能只改了一半
- 其他线程再访问会崩溃

所以大多数程序应该避免：

```c
PTHREAD_CANCEL_ASYNCHRONOUS
```

更安全的做法：

1. 使用延迟取消
2. 在循环中手动调用 `pthread_testcancel()`
3. 使用 cleanup handler 保证资源释放
4. 或者不用 pthread_cancel，改用共享标志位让线程自行退出

---

## 6.9 更推荐的停止线程方式：退出标志

很多实际项目不用 `pthread_cancel()`，而是：

```c
volatile int stop = 0;
```

更严谨时用原子变量或加锁。

示例：

```c
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

atomic_int stop = 0;

void *worker(void *arg) {
    while (!atomic_load(&stop)) {
        printf("working...\n");
        sleep(1);
    }

    printf("worker exits normally.\n");
    return NULL;
}

int main(void) {
    pthread_t tid;

    pthread_create(&tid, NULL, worker, NULL);

    sleep(3);
    atomic_store(&stop, 1);

    pthread_join(tid, NULL);
    return 0;
}
```

这种方式的优点是：

- 线程在安全位置退出
- 资源释放逻辑更清晰
- 不容易造成锁和数据结构损坏

---

# 7. 第33章：线程更多细节

对应章节：

- 33.1 线程栈
- 33.2 线程和信号
- 33.2.1 UNIX 信号模型如何映射到线程中
- 33.2.2 操作线程信号掩码
- 33.2.3 向线程发送信号
- 33.2.4 妥善处理异步信号
- 33.3 线程和进程控制
- 33.4 线程实现模型
- 33.5 Linux POSIX 线程的实现
- 33.5.1 LinuxThreads
- 33.5.2 NPTL
- 33.5.3 哪一种线程实现
- 33.6 Pthread API 的高级特性
- 33.7 总结
- 33.8 练习

---

## 7.1 线程栈

每个线程都有自己的栈。

栈上存放：

- 函数调用帧
- 局部变量
- 返回地址
- 部分临时数据

例如：

```c
void *worker(void *arg) {
    int local = 100; // 在该线程自己的栈上
    return NULL;
}
```

每个线程都有自己的 `local`。

---

## 7.2 线程栈大小问题

如果你在线程函数里定义超大局部数组：

```c
char big[100 * 1024 * 1024];
```

可能栈溢出，程序崩溃。

解决方法：

1. 使用 `malloc()` 放到堆上
2. 调整线程栈大小
3. 避免深递归

设置栈大小：

```c
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setstacksize(&attr, 2 * 1024 * 1024);
pthread_create(&tid, &attr, worker, NULL);
pthread_attr_destroy(&attr);
```

---

## 7.3 线程和信号：先记住三句话

1. 信号处理函数是进程级共享的。
2. 信号掩码是每个线程各自拥有的。
3. 进程定向信号会投递给某个没有阻塞该信号的线程。

这部分容易混乱，下面慢慢解释。

---

## 7.4 UNIX 信号模型如何映射到线程

传统进程模型中：

```text
信号 -> 进程
```

多线程后：

```text
信号 -> 进程中的某个线程处理
```

但不是所有信号都一样。

### 进程定向信号

例如：

```bash
kill -TERM pid
```

这是发给整个进程的。内核会选择一个没有阻塞该信号的线程处理。

### 线程定向信号

例如：

```c
pthread_kill(tid, SIGUSR1);
```

这是发给指定线程的。

---

## 7.5 线程信号掩码

每个线程都有自己的信号掩码。

多线程程序中应使用：

```c
pthread_sigmask()
```

而不是 `sigprocmask()`。

常见设计：

1. 主线程启动时阻塞某些信号。
2. 创建一个专门的信号处理线程。
3. 信号处理线程用 `sigwait()` 同步等待信号。
4. 其他工作线程不直接处理异步信号。

这样更稳定。

---

## 7.6 示例19：专门线程处理信号

```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

void *signal_thread(void *arg) {
    sigset_t *set = (sigset_t *)arg;
    int sig;

    while (1) {
        int ret = sigwait(set, &sig);
        if (ret == 0) {
            printf("Signal thread got signal %d\n", sig);
            if (sig == SIGTERM || sig == SIGINT) {
                break;
            }
        }
    }

    return NULL;
}

int main(void) {
    pthread_t tid;
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);

    // 所有后续创建的线程都会继承这个信号掩码
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    pthread_create(&tid, NULL, signal_thread, &set);

    printf("Process pid = %d\n", getpid());
    printf("Press Ctrl+C to send SIGINT.\n");

    pthread_join(tid, NULL);

    printf("Exit gracefully.\n");
    return 0;
}
```

这个模式的好处：

- 不在异步信号处理函数中做复杂事
- 避免信号随机打断工作线程
- 退出逻辑更可控

---

## 7.7 向线程发送信号

```c
pthread_kill(pthread_t thread, int sig);
```

它不是“杀死线程”的意思，而是向某个线程发送信号。

例如：

```c
pthread_kill(tid, SIGUSR1);
```

如果 `sig` 是 0，则只做错误检查，不发送信号，常用于检查线程是否存在。

---

## 7.8 妥善处理异步信号

信号处理函数里能安全调用的函数很少。

不推荐在 signal handler 里做：

```c
printf()
malloc()
pthread_mutex_lock()
```

更安全的设计：

1. handler 里只设置一个 `sig_atomic_t` 标志
2. 主循环检测这个标志
3. 或使用 `sigwait()` 让专门线程处理信号

示例：

```c
volatile sig_atomic_t got_sigint = 0;

void handler(int sig) {
    got_sigint = 1;
}
```

---

## 7.9 线程和进程控制

### `fork()` 和多线程

多线程进程调用 `fork()` 后，子进程里通常只有调用 `fork()` 的那个线程存在。

这会带来问题：

- 其他线程消失了
- 但它们持有的锁状态可能还在
- 子进程如果继续使用这些锁，可能死锁

所以多线程程序中，`fork()` 后最好尽快 `exec()`。

常见原则：

```text
多线程程序 fork 后，在子进程中只调用 async-signal-safe 函数，然后 exec。
```

---

## 7.10 `pthread_atfork()`

如果必须在多线程程序中 fork，可以注册 fork 处理函数：

```c
pthread_atfork(prepare, parent, child);
```

| 函数 | 调用时机 |
|---|---|
| `prepare` | fork 前，在父进程中调用 |
| `parent` | fork 后，在父进程中调用 |
| `child` | fork 后，在子进程中调用 |

用途：

- fork 前锁住全局锁
- fork 后父进程解锁
- fork 后子进程重新初始化锁

但这比较高级，初学阶段记住风险即可。

---

## 7.11 线程实现模型

理论上线程实现模型有几种：

### 多对一

多个用户线程映射到一个内核线程。

```text
多个用户线程 -> 1 个内核线程
```

优点：用户态切换快。  
缺点：一个线程阻塞，整个进程都可能阻塞；不能真正多核并行。

---

### 一对一

一个用户线程对应一个内核线程。

```text
1 个用户线程 -> 1 个内核线程
```

优点：能利用多核，一个线程阻塞不影响其他线程。  
缺点：创建和调度开销较大。

Linux NPTL 使用的就是一对一模型。

---

### 多对多

多个用户线程映射到多个内核线程。

```text
多个用户线程 -> 多个内核线程
```

设计复杂，现代 Linux pthread 主要不是这种模型。

---

## 7.12 Linux POSIX 线程实现

Linux 早期有 LinuxThreads，后来主要使用 NPTL。

你可以通过：

```bash
getconf GNU_LIBPTHREAD_VERSION
```

查看系统 pthread 实现。

现代 Linux 一般是 NPTL。

NPTL 的特点：

- 基于内核线程
- 与 POSIX 更兼容
- 性能更好
- 每个 pthread 通常对应一个内核调度实体

---

## 7.13 Pthread 高级特性

后续可以继续学习：

- 读写锁 `pthread_rwlock_t`
- 自旋锁 `pthread_spinlock_t`
- 屏障 `pthread_barrier_t`
- 线程调度属性
- robust mutex
- priority inheritance mutex
- CPU affinity
- futex

初学建议优先掌握：

1. `pthread_create`
2. `pthread_join`
3. `pthread_detach`
4. `pthread_mutex`
5. `pthread_cond`
6. 线程安全
7. 线程退出与资源清理

---

# 8. 常见面试/考试题速记

## 8.1 pipe 和 FIFO 有什么区别？

| 对比 | pipe | FIFO |
|---|---|---|
| 是否有路径名 | 没有 | 有 |
| 是否适合无亲缘关系进程 | 不方便 | 适合 |
| 创建方式 | `pipe()` | `mkfifo()` |
| 使用方式 | 文件描述符 | 像打开文件一样 `open()` |
| 本质 | 内核管道 | 命名的内核管道 |

---

## 8.2 管道为什么要关闭不用的一端？

因为 EOF 只有在所有写端关闭后才会出现。

如果读进程自己也保留了写端，读端可能一直阻塞。

---

## 8.3 进程和线程的区别？

核心区别：

> 进程是资源分配单位，线程是 CPU 调度执行单位。

进程地址空间独立，线程共享同一进程地址空间。

---

## 8.4 `pthread_join()` 和 `pthread_detach()` 区别？

| 操作 | 含义 |
|---|---|
| join | 等线程结束，获取返回值，回收资源 |
| detach | 线程结束自动回收资源，不可 join |

口诀：

> 一个线程最终要么被 join，要么被 detach。

---

## 8.5 mutex 和 condition variable 区别？

| 工具 | 解决的问题 |
|---|---|
| mutex | 同一时间只允许一个线程进入临界区 |
| condition variable | 条件不满足时睡眠，条件变化时被通知 |

mutex 是“锁门”，condition variable 是“等通知”。

---

## 8.6 为什么 `pthread_cond_wait()` 要放在 while 里？

因为被唤醒不等于条件一定满足。

原因包括：

- 虚假唤醒
- 多个线程竞争
- 条件被其他线程抢先改变

正确模式：

```c
pthread_mutex_lock(&mutex);
while (!predicate) {
    pthread_cond_wait(&cond, &mutex);
}
pthread_mutex_unlock(&mutex);
```

---

## 8.7 什么是死锁？怎么避免？

死锁是多个线程互相等待对方持有的锁。

避免方法：

- 固定加锁顺序
- 减少锁数量
- 临界区尽量短
- 使用 trylock
- 不要持锁调用复杂未知代码

---

## 8.8 线程安全和可重入的区别？

线程安全：多个线程同时调用也正确。

可重入：函数执行中被打断后再次进入仍然正确。

可重入通常要求更严格。

---

## 8.9 TSD 和 TLS 区别？

| 对比 | TSD | TLS |
|---|---|---|
| 使用方式 | pthread key API | 变量声明 |
| 动态性 | 强 | 弱 |
| 析构 | 支持 destructor | 视实现而定 |
| 适合 | 库函数、动态资源 | 普通每线程变量 |

---

## 8.10 为什么不推荐异步取消线程？

因为线程可能在任意位置被终止：

- 锁没释放
- 内存泄漏
- 数据结构损坏
- 文件状态不一致

更推荐延迟取消或退出标志。

---

# 9. 学习路线与练习建议

## 9.1 推荐学习顺序

第一阶段：基础进程通信

1. `fork()`
2. `pipe()`
3. `dup2()`
4. `exec()`
5. FIFO

第二阶段：线程基础

1. `pthread_create`
2. `pthread_join`
3. `pthread_detach`
4. 线程返回值
5. 线程属性

第三阶段：线程同步

1. 数据竞争
2. mutex
3. 死锁
4. condition variable
5. 生产者消费者

第四阶段：线程安全

1. 可重入
2. 静态 buffer 的问题
3. `pthread_once`
4. TSD
5. TLS

第五阶段：高级细节

1. 线程取消
2. cleanup handler
3. 线程与信号
4. fork 与多线程
5. 栈大小

---

## 9.2 必做练习

### 练习1：pipe 双向通信

使用两个 pipe 实现：

```text
父进程 -> 子进程：发送一个整数
子进程 -> 父进程：返回这个整数的平方
```

提示：

- pipe1 父写子读
- pipe2 子写父读

---

### 练习2：FIFO 聊天

写两个程序：

- 程序 A 从 `/tmp/fifo_a` 读，从 `/tmp/fifo_b` 写
- 程序 B 从 `/tmp/fifo_b` 读，从 `/tmp/fifo_a` 写

实现简单互发消息。

---

### 练习3：多线程求和

创建 4 个线程，计算 1 到 1000000 的和。

要求：

- 每个线程计算一段范围
- 主线程 join 后合并结果
- 不共享写同一个 sum，避免锁

---

### 练习4：互斥量保护链表

多个线程同时向一个链表插入节点。

要求：

- 不加锁观察问题
- 加 mutex 修复
- 尝试减小临界区

---

### 练习5：生产者消费者

基于本笔记示例扩展：

- 2 个生产者
- 3 个消费者
- 生产完后让消费者正常退出

提示：

- 增加 `done` 标志
- 消费者等待条件应为：`count > 0 || done`

---

### 练习6：线程池

实现一个简单线程池：

- 主线程提交任务
- 工作线程从队列取任务
- 队列为空时等待条件变量
- 程序退出时 broadcast 唤醒所有线程

这个练习能把 mutex 和 condition variable 真正串起来。

---

# 10. 最后总结

## 10.1 IPC 的核心

IPC 不是死记 API，而是理解：

> 进程地址空间隔离，所以进程之间需要内核帮忙传数据或协调时机。

管道和 FIFO 的关键是：

- 读端、写端
- 阻塞行为
- EOF 条件
- 关闭不用的一端
- `dup2()` 重定向

---

## 10.2 线程的核心

线程不是“更高级的进程”，而是：

> 同一个进程里的多个执行流，共享资源，所以更轻量，也更容易出错。

线程编程三大重点：

1. 生命周期：create、join、detach、exit
2. 同步：mutex、condition variable
3. 安全：线程安全、TSD、TLS、取消清理

---

## 10.3 最重要的几个口诀

1. **进程通信靠 IPC，线程通信靠共享内存。**
2. **共享变量不加锁，结果迟早会出错。**
3. **创建线程后，要么 join，要么 detach。**
4. **条件变量等待必须用 while。**
5. **不要持锁做慢操作。**
6. **多把锁必须固定加锁顺序。**
7. **线程取消要清理资源。**
8. **多线程程序 fork 后最好马上 exec。**

---

## 10.4 建议你真正掌握的代码模板

### 线程创建模板

```c
pthread_t tid;
pthread_create(&tid, NULL, worker, arg);
pthread_join(tid, NULL);
```

### mutex 模板

```c
pthread_mutex_lock(&mutex);
// critical section
pthread_mutex_unlock(&mutex);
```

### condition variable 模板

```c
pthread_mutex_lock(&mutex);

while (!predicate) {
    pthread_cond_wait(&cond, &mutex);
}

// use shared state

pthread_mutex_unlock(&mutex);
```

### pipe 模板

```c
int fd[2];
pipe(fd);

if (fork() == 0) {
    close(fd[1]);
    read(fd[0], buf, size);
} else {
    close(fd[0]);
    write(fd[1], data, len);
}
```

---

> 学 Linux 编程最有效的方法不是只看概念，而是：  
> **每个 API 都写一个最小例子，然后故意制造错误，再修复它。**  
> 比如：不关闭管道端、不加锁计数、条件变量用 if、持锁后取消线程。  
> 你亲眼看到程序卡住、结果错误、死锁，再理解正确写法，会记得非常牢。
