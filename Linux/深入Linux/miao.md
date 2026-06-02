下面这组练习基于你上传的第 6、20、21 章内容设计，按顺序做会比较好：从“进程是什么”开始，逐步练到“信号处理器怎么安全地写”。

默认环境：Linux / WSL / macOS 终端，使用 C 语言和 POSIX API。

通用编译方式：

```bash
gcc -Wall -Wextra -O0 -g xxx.c -o xxx
```

---

# 练习 1：写一个进程身份查看器

## 目标

理解：

* 程序和进程的区别
* PID
* PPID
* 一个程序可以运行成多个进程
* shell 是你程序的父进程

## 任务

写一个程序，启动后打印：

1. 当前进程 PID；
2. 父进程 PPID；
3. 每秒打印一次 “I am alive”；
4. 让程序运行一段时间，方便你用 `ps` 查看。

## 示例代码：`proc_id.c`

```c
#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("My PID  = %d\n", getpid());
    printf("My PPID = %d\n", getppid());

    for (int i = 0; i < 10; i++) {
        printf("[%d] I am alive\n", i);
        sleep(1);
    }

    return 0;
}
```

编译运行：

```bash
gcc -Wall -Wextra -O0 -g proc_id.c -o proc_id
./proc_id
```

另开一个终端查看：

```bash
ps -ef | grep proc_id
```

## 运行逻辑

当你执行：

```bash
./proc_id
```

磁盘上的 `proc_id` 是程序文件。运行起来之后，操作系统会为它创建一个进程。

程序内部：

```c
getpid()
```

获取当前进程编号。

```c
getppid()
```

获取父进程编号。通常这个父进程就是你的 shell，比如 `bash`、`zsh`。

## 生产中的用处

真实服务里经常会把 PID 打进日志，例如：

```text
worker started, pid=12345
```

这样排查问题时可以知道：

* 哪个进程在运行；
* 哪个进程占用 CPU；
* 哪个进程需要被重启；
* 哪个进程收到了信号；
* 多进程服务里是哪一个 worker 出错。

---

# 练习 2：观察进程内存布局

## 目标

理解：

* text 段
* data 段
* BSS 段
* heap 堆
* stack 栈
* 命令行参数和环境变量大概在高地址区域
* 虚拟地址空间

## 任务

写一个程序，打印不同变量的地址。

## 示例代码：`memory_layout.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int global_init = 100;       // data 段：已初始化全局变量
int global_uninit;           // BSS 段：未初始化全局变量

void func(void) {
    int local_func = 20;     // 栈
    printf("Address of local_func       = %p\n", (void *)&local_func);
}

int main(int argc, char *argv[], char *envp[]) {
    static int static_init = 200;   // data 段
    static int static_uninit;       // BSS 段

    int local_main = 10;            // 栈
    int *heap_var = malloc(sizeof(int)); // 堆

    if (heap_var == NULL) {
        perror("malloc");
        return 1;
    }

    *heap_var = 300;

    printf("PID = %d\n\n", getpid());

    printf("Text/code function address  = %p\n", (void *)main);
    printf("global_init address         = %p\n", (void *)&global_init);
    printf("global_uninit address       = %p\n", (void *)&global_uninit);
    printf("static_init address         = %p\n", (void *)&static_init);
    printf("static_uninit address       = %p\n", (void *)&static_uninit);
    printf("heap_var address            = %p\n", (void *)heap_var);
    printf("local_main address          = %p\n", (void *)&local_main);

    func();

    printf("argv address                = %p\n", (void *)argv);
    printf("argv[0] address             = %p\n", (void *)argv[0]);
    printf("envp address                = %p\n", (void *)envp);

    free(heap_var);
    return 0;
}
```

编译运行：

```bash
gcc -Wall -Wextra -O0 -g memory_layout.c -o memory_layout
./memory_layout
```

## 运行逻辑

你会看到类似：

```text
Text/code function address  = 0x55...
global_init address         = 0x55...
global_uninit address       = 0x55...
heap_var address            = 0x55...
local_main address          = 0x7f...
```

大致可以这样理解：

```text
低地址
text/code
data
BSS
heap
...
stack
argv/envp
高地址
```

不过不同系统、编译选项、ASLR 机制会让地址看起来不同。

## 进一步实验

连续运行两次：

```bash
./memory_layout
./memory_layout
```

你可能会发现地址不完全一样。这和地址空间布局随机化 ASLR 有关。

但重点是：

每个进程看到的是自己的虚拟地址空间。两个进程中变量地址看起来可能类似，但它们不是同一块真实物理内存。

## 生产中的用处

理解内存布局对这些场景非常重要：

1. 排查段错误 `Segmentation fault`；
2. 理解栈溢出；
3. 分析内存泄漏；
4. 理解 `malloc/free`；
5. 使用调试器 `gdb`；
6. 阅读 core dump；
7. 理解为什么进程之间不能直接访问彼此变量。

---

# 练习 3：观察栈帧和递归调用

## 目标

理解：

* 函数调用会创建栈帧
* 局部变量一般在栈上
* 每层递归都会消耗新的栈空间
* 递归太深可能栈溢出

## 任务

写一个递归函数，每次打印当前递归深度和局部变量地址。

## 示例代码：`stack_frame.c`

```c
#include <stdio.h>
#include <unistd.h>

void recursive(int depth) {
    int local = depth;

    printf("depth = %d, local address = %p\n",
           depth, (void *)&local);

    sleep(1);

    if (depth < 5) {
        recursive(depth + 1);
    }

    printf("returning from depth = %d\n", depth);
}

int main(void) {
    recursive(1);
    return 0;
}
```

编译运行：

```bash
gcc -Wall -Wextra -O0 -g stack_frame.c -o stack_frame
./stack_frame
```

## 运行逻辑

调用过程是：

```text
main()
  recursive(1)
    recursive(2)
      recursive(3)
        recursive(4)
          recursive(5)
```

每调用一次函数，系统就在栈上创建一个新的栈帧。

当最深层函数返回时：

```text
recursive(5) 返回
recursive(4) 返回
recursive(3) 返回
recursive(2) 返回
recursive(1) 返回
main 返回
```

## 修改实验

把：

```c
if (depth < 5)
```

改成：

```c
if (depth < 1000000)
```

程序很可能会崩溃：

```text
Segmentation fault
```

这就是栈溢出。

## 生产中的用处

真实项目中，栈帧知识用于：

1. 分析函数调用链；
2. 读懂调试器中的 backtrace；
3. 避免无限递归；
4. 设计递归算法时控制深度；
5. 理解为什么大数组不要随便放在局部变量里。

例如：

```c
void f(void) {
    char buf[10000000];
}
```

这种大数组放在栈上，很容易导致栈溢出。生产代码中更常用堆：

```c
char *buf = malloc(10000000);
```

---

# 练习 4：命令行参数和环境变量练习

## 目标

理解：

* `argc`
* `argv`
* `getenv`
* `setenv`
* `unsetenv`
* 环境变量是进程配置的一部分

## 任务

写一个小工具：

```bash
./show_config name age
```

它打印：

1. 所有命令行参数；
2. 当前用户 HOME；
3. PATH；
4. 自己设置一个环境变量 `MY_APP_MODE=debug`；
5. 再读取它。

## 示例代码：`show_config.c`

```c
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    printf("Argument count: argc = %d\n\n", argc);

    for (int i = 0; i < argc; i++) {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    printf("\n");

    char *home = getenv("HOME");
    char *path = getenv("PATH");

    if (home != NULL) {
        printf("HOME = %s\n", home);
    } else {
        printf("HOME is not set\n");
    }

    if (path != NULL) {
        printf("PATH = %s\n", path);
    } else {
        printf("PATH is not set\n");
    }

    printf("\nSetting MY_APP_MODE...\n");

    if (setenv("MY_APP_MODE", "debug", 1) == -1) {
        perror("setenv");
        return 1;
    }

    char *mode = getenv("MY_APP_MODE");

    if (mode != NULL) {
        printf("MY_APP_MODE = %s\n", mode);
    }

    unsetenv("MY_APP_MODE");

    mode = getenv("MY_APP_MODE");

    if (mode == NULL) {
        printf("MY_APP_MODE has been removed\n");
    }

    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g show_config.c -o show_config
./show_config Alice 18
```

## 运行逻辑

执行：

```bash
./show_config Alice 18
```

对应：

```text
argc = 3
argv[0] = ./show_config
argv[1] = Alice
argv[2] = 18
```

环境变量不是你在代码里写死的，而是进程启动时从父进程继承来的。

比如 shell 中：

```bash
MY_APP_MODE=release ./show_config Alice 18
```

这个环境变量会传给当前程序。

## 生产中的用处

环境变量在生产里非常常见：

```bash
PORT=8080 ./server
DB_HOST=127.0.0.1 ./server
APP_ENV=production ./server
```

很多服务不会把配置写死在代码里，而是通过环境变量传入：

* 数据库地址；
* 服务端口；
* 日志级别；
* 运行环境；
* 密钥路径；
* 开关配置。

这就是为什么后端服务、Docker、Kubernetes 都大量使用环境变量。

---

# 练习 5：用 setjmp/longjmp 做错误恢复

## 目标

理解：

* `setjmp()` 会保存执行位置
* `longjmp()` 可以跨函数跳回去
* `setjmp()` 可能返回两次
* 这种机制可以模拟简单异常处理

## 任务

模拟一个配置加载流程：

```text
main
  load_config
    parse_config
      check_config
```

如果底层发现配置错误，直接跳回 main。

## 示例代码：`jump_error.c`

```c
#include <stdio.h>
#include <setjmp.h>

static jmp_buf env;

void check_config(int value) {
    if (value <= 0) {
        printf("check_config: invalid config value\n");
        longjmp(env, 1);
    }

    printf("check_config: config is valid\n");
}

void parse_config(int value) {
    printf("parse_config: parsing...\n");
    check_config(value);
    printf("parse_config: done\n");
}

void load_config(int value) {
    printf("load_config: loading...\n");
    parse_config(value);
    printf("load_config: done\n");
}

int main(void) {
    int ret = setjmp(env);

    if (ret == 0) {
        printf("main: first time through setjmp\n");
        load_config(-1);
        printf("main: config loaded successfully\n");
    } else {
        printf("main: recovered from config error, ret = %d\n", ret);
    }

    printf("main: program continues\n");

    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g jump_error.c -o jump_error
./jump_error
```

## 运行逻辑

第一次执行：

```c
setjmp(env)
```

返回 `0`，程序进入正常流程。

后来在深层函数里：

```c
longjmp(env, 1)
```

程序直接跳回：

```c
setjmp(env)
```

这一次 `setjmp()` 返回 `1`。

所以输出会类似：

```text
main: first time through setjmp
load_config: loading...
parse_config: parsing...
check_config: invalid config value
main: recovered from config error, ret = 1
main: program continues
```

注意：

```c
printf("parse_config: done\n");
printf("load_config: done\n");
```

不会执行，因为流程被跳过了。

## 生产中的用处

`setjmp/longjmp` 在普通业务代码中不推荐大量使用，因为它会破坏正常调用结构。

但它在一些底层场景有用：

1. 解释器；
2. 脚本语言运行时；
3. 错误恢复框架；
4. C 语言中模拟异常；
5. 从深层解析器中快速退出；
6. 某些信号恢复场景。

你可以把它理解成“安全出口”，不是普通楼梯。

---

# 练习 6：捕获 Ctrl+C，理解 signal()

## 目标

理解：

* 信号是异步通知
* Ctrl+C 会产生 `SIGINT`
* `signal()` 可以改变信号处置
* 信号处理器会打断正常执行流程

## 任务

写一个程序，每秒打印一次 `Working...`，按 Ctrl+C 时不退出，而是打印收到信号。

## 示例代码：`catch_sigint_signal.c`

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    printf("Caught signal: %d\n", sig);
}

int main(void) {
    signal(SIGINT, handler);

    while (1) {
        printf("Working...\n");
        sleep(1);
    }

    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g catch_sigint_signal.c -o catch_sigint_signal
./catch_sigint_signal
```

按：

```text
Ctrl+C
```

你会看到：

```text
Caught signal: 2
```

## 运行逻辑

程序原本在执行：

```c
sleep(1);
```

或者：

```c
printf("Working...\n");
```

信号突然到达，操作系统打断当前流程，转去执行：

```c
handler(SIGINT);
```

handler 返回后，程序继续原来的流程。

## 重要提醒

这个练习是为了入门理解。

但生产中不建议在 signal handler 里写：

```c
printf(...)
```

因为 `printf()` 不是异步信号安全函数。

更好的方式是：

1. handler 只设置标志位；
2. 主循环看到标志后再执行复杂逻辑。

后面的练习会写安全版本。

## 生产中的用处

信号捕获常用于：

1. 服务优雅退出；
2. 重新加载配置；
3. 接收运维控制命令；
4. 守护进程响应系统事件；
5. 容器服务处理 `SIGTERM`。

例如 Kubernetes 停止容器时通常会先给进程发送 `SIGTERM`，服务应该捕获它并优雅退出。

---

# 练习 7：用 kill、raise、kill(pid, 0) 做进程控制

## 目标

理解：

* `kill()` 的本质是发送信号，不一定是杀进程
* `raise()` 给自己发送信号
* `kill(pid, 0)` 可以检查进程是否存在
* `strsignal()` 可以显示信号描述

## 任务

写一个工具：

```bash
./send_signal <pid> <signal_number>
```

例如：

```bash
./send_signal 12345 15
```

向 PID 为 12345 的进程发送 `SIGTERM`。

## 示例代码：`send_signal.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <pid> <signal_number>\n", argv[0]);
        return 1;
    }

    pid_t pid = (pid_t)atoi(argv[1]);
    int sig = atoi(argv[2]);

    printf("Checking process %d...\n", pid);

    if (kill(pid, 0) == -1) {
        if (errno == ESRCH) {
            printf("Process does not exist\n");
        } else if (errno == EPERM) {
            printf("Process exists, but no permission\n");
        } else {
            perror("kill(pid, 0)");
        }
        return 1;
    }

    printf("Process exists. Sending signal %d (%s)\n",
           sig, strsignal(sig));

    if (kill(pid, sig) == -1) {
        perror("kill");
        return 1;
    }

    printf("Signal sent successfully\n");

    return 0;
}
```

编译：

```bash
gcc -Wall -Wextra -O0 -g send_signal.c -o send_signal
```

配合练习 6 使用：

终端 1：

```bash
./catch_sigint_signal
```

看到它的 PID：

```bash
ps -ef | grep catch_sigint_signal
```

终端 2：

```bash
./send_signal <pid> 2
```

`2` 通常是 `SIGINT`。

## `raise()` 小练习

再写一个自己给自己发信号的程序。

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    printf("I sent signal %d to myself\n", sig);
}

int main(void) {
    signal(SIGUSR1, handler);

    printf("PID = %d\n", getpid());
    printf("Raising SIGUSR1...\n");

    raise(SIGUSR1);

    printf("Program continues\n");

    return 0;
}
```

## 生产中的用处

`kill(pid, 0)` 很常用于守护进程脚本：

```text
检查服务是否还活着
检查 pidfile 中的 PID 是否有效
检查当前用户有没有权限控制该进程
```

`kill()` 则用于：

1. 通知服务退出；
2. 通知服务重载配置；
3. 给 worker 进程发送控制命令；
4. 实现父子进程之间的简单通信。

---

# 练习 8：用 sigaction 写安全的优雅退出

## 目标

理解：

* `sigaction()` 比 `signal()` 更推荐
* handler 应尽量短
* 使用 `volatile sig_atomic_t`
* 复杂逻辑放在主循环
* 捕获 `SIGINT` 和 `SIGTERM`

## 任务

写一个长期运行的 worker：

1. 每秒处理一个任务；
2. 按 Ctrl+C 或收到 `SIGTERM` 时，不要立即硬退出；
3. 设置退出标志；
4. 主循环检测标志后优雅退出。

## 示例代码：`graceful_worker.c`

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t stop = 0;

void handler(int sig) {
    (void)sig;
    stop = 1;
}

int main(void) {
    struct sigaction sa;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        return 1;
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
        return 1;
    }

    printf("Worker started. PID = %d\n", getpid());
    printf("Press Ctrl+C or send SIGTERM to stop.\n");

    int job_id = 1;

    while (!stop) {
        printf("Processing job %d...\n", job_id++);
        sleep(1);
    }

    printf("Stop flag detected. Cleaning up...\n");
    sleep(1);
    printf("Worker exited gracefully.\n");

    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g graceful_worker.c -o graceful_worker
./graceful_worker
```

终端 2：

```bash
kill -TERM <pid>
```

## 运行逻辑

handler 只做一件事：

```c
stop = 1;
```

这非常重要。

不要在 handler 里做复杂操作，例如：

```c
printf()
malloc()
free()
fclose()
exit()
```

主循环负责真正退出：

```c
while (!stop) {
    ...
}

printf("Cleaning up...\n");
```

## 为什么要用 `volatile sig_atomic_t`

```c
volatile sig_atomic_t stop;
```

含义是：

1. `sig_atomic_t`：读写这个类型适合在信号处理器和主程序之间共享；
2. `volatile`：告诉编译器这个变量可能被异步改变，不要错误优化。

## 生产中的用处

这是最经典的服务退出模式。

真实服务收到：

```text
SIGTERM
```

时通常需要：

1. 停止接收新请求；
2. 处理完当前请求；
3. 刷新日志；
4. 关闭 socket；
5. 释放资源；
6. 正常退出。

如果直接被 `SIGKILL` 杀死，就没有机会清理资源。

---

# 练习 9：信号掩码、pending 信号和标准信号不排队

## 目标

理解：

* `sigset_t`
* `sigemptyset`
* `sigaddset`
* `sigprocmask`
* 阻塞信号
* pending 信号
* 标准信号通常不排队

## 任务

程序先阻塞 `SIGINT`，然后让你在 10 秒内多次按 Ctrl+C。之后查看 `SIGINT` 是否 pending，再解除阻塞。

## 示例代码：`signal_mask_pending.c`

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "SIGINT handled\n", 15);
}

int main(void) {
    struct sigaction sa;
    sigset_t block_set;
    sigset_t pending_set;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    sigemptyset(&block_set);
    sigaddset(&block_set, SIGINT);

    printf("Blocking SIGINT. Press Ctrl+C many times within 10 seconds.\n");

    if (sigprocmask(SIG_BLOCK, &block_set, NULL) == -1) {
        perror("sigprocmask block");
        return 1;
    }

    sleep(10);

    if (sigpending(&pending_set) == -1) {
        perror("sigpending");
        return 1;
    }

    if (sigismember(&pending_set, SIGINT)) {
        printf("SIGINT is pending\n");
    } else {
        printf("SIGINT is not pending\n");
    }

    printf("Unblocking SIGINT now...\n");

    if (sigprocmask(SIG_UNBLOCK, &block_set, NULL) == -1) {
        perror("sigprocmask unblock");
        return 1;
    }

    sleep(2);

    printf("Program exits\n");

    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g signal_mask_pending.c -o signal_mask_pending
./signal_mask_pending
```

在 10 秒内多按几次 Ctrl+C。

## 运行逻辑

程序先执行：

```c
sigprocmask(SIG_BLOCK, &block_set, NULL);
```

这表示暂时阻塞 `SIGINT`。

你按 Ctrl+C 后，`SIGINT` 不会马上递送，而是进入 pending 状态。

之后：

```c
sigpending(&pending_set);
```

可以查看当前有哪些信号在等待。

最后解除阻塞：

```c
sigprocmask(SIG_UNBLOCK, &block_set, NULL);
```

这时 pending 的 `SIGINT` 才会被递送。

## 标准信号不排队

你可能按了 5 次 Ctrl+C，但解除阻塞后通常只看到一次：

```text
SIGINT handled
```

这说明标准信号通常只记录“发生过”，不精确记录“发生了几次”。

## 生产中的用处

信号适合表达状态变化：

```text
请退出
请重载配置
子进程结束了
定时器到了
```

不适合表达精确计数：

```text
任务来了 37 次
消息到了 100 个
```

如果要可靠计数，应考虑：

1. pipe；
2. socket；
3. eventfd；
4. 消息队列；
5. 共享内存加同步机制；
6. 实时信号。

---

# 练习 10：pause 等待信号

## 目标

理解：

* `pause()` 会让进程睡眠等待信号
* 信号处理器返回后，`pause()` 返回
* `pause()` 常见返回值是 `-1`
* `errno` 通常是 `EINTR`

## 任务

写一个程序，启动后什么都不做，只等待 `SIGUSR1`。

## 示例代码：`pause_wait.c`

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

static volatile sig_atomic_t got_usr1 = 0;

void handler(int sig) {
    (void)sig;
    got_usr1 = 1;
}

int main(void) {
    struct sigaction sa;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("PID = %d\n", getpid());
    printf("Waiting for SIGUSR1...\n");

    while (!got_usr1) {
        int ret = pause();

        if (ret == -1 && errno == EINTR) {
            printf("pause interrupted by signal\n");
        }
    }

    printf("SIGUSR1 received. Program exits.\n");

    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g pause_wait.c -o pause_wait
./pause_wait
```

另一个终端：

```bash
kill -USR1 <pid>
```

## 运行逻辑

主程序执行：

```c
pause();
```

此时进程睡眠。

收到 `SIGUSR1` 后：

1. 操作系统唤醒进程；
2. 执行 handler；
3. handler 设置 `got_usr1 = 1`；
4. handler 返回；
5. `pause()` 返回；
6. 主循环发现标志已变，退出。

## 生产中的用处

`pause()` 常用于简单事件等待程序。

不过在复杂生产服务中，更常见的是：

1. `select`
2. `poll`
3. `epoll`
4. `signalfd`
5. event loop

但理解 `pause()` 可以帮助你理解信号如何打断进程。

---

# 练习 11：SA_SIGINFO 获取发送者信息

## 目标

理解：

* 普通 handler 只能拿到信号编号
* `SA_SIGINFO` 可以获得更多信息
* `siginfo_t` 里有发送者 PID、UID 等信息
* handler 仍然要尽量简单

## 任务

写一个程序，接收 `SIGUSR1`，记录是谁发送的信号。

## 示例代码：`siginfo_demo.c`

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t got_signal = 0;
static volatile sig_atomic_t sender_pid = 0;
static volatile sig_atomic_t sender_uid = 0;

void handler(int sig, siginfo_t *info, void *ucontext) {
    (void)sig;
    (void)ucontext;

    got_signal = 1;
    sender_pid = info->si_pid;
    sender_uid = info->si_uid;
}

int main(void) {
    struct sigaction sa;

    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("Receiver PID = %d\n", getpid());
    printf("Send signal with: kill -USR1 %d\n", getpid());

    while (!got_signal) {
        pause();
    }

    printf("Received SIGUSR1\n");
    printf("Sender PID = %d\n", sender_pid);
    printf("Sender UID = %d\n", sender_uid);

    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g siginfo_demo.c -o siginfo_demo
./siginfo_demo
```

另一个终端：

```bash
kill -USR1 <pid>
```

## 运行逻辑

普通 handler 是：

```c
void handler(int sig)
```

只能知道哪个信号来了。

使用 `SA_SIGINFO` 后，handler 变成：

```c
void handler(int sig, siginfo_t *info, void *ucontext)
```

其中：

```c
info->si_pid
```

表示发送信号的进程 PID。

```c
info->si_uid
```

表示发送信号的用户 ID。

## 为什么不在 handler 里 printf？

虽然你可以写：

```c
printf("Sender PID = %d\n", info->si_pid);
```

但这不是安全习惯。

更好的方式是：

1. handler 保存简单信息；
2. 主程序打印详细信息。

## 生产中的用处

`SA_SIGINFO` 在这些场景有用：

1. 调试工具；
2. 进程监控；
3. 安全审计；
4. 崩溃诊断；
5. 区分信号来源；
6. 处理 `SIGCHLD` 时获取子进程信息；
7. 处理 `SIGSEGV` 时获取错误地址。

---

# 练习 12：系统调用被信号中断 EINTR

## 目标

理解：

* 信号可能打断阻塞系统调用
* 被打断后可能返回 `-1`
* `errno == EINTR`
* `SA_RESTART` 可以让部分系统调用自动重启
* 健壮代码要处理 `EINTR`

## 任务

程序等待用户输入，同时允许另一个终端发送信号打断它。

## 示例代码：`eintr_read.c`

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void handler(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\nSignal caught\n", 15);
}

int main(void) {
    struct sigaction sa;
    char buf[128];

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);

    /*
       先设置为 0，观察 read 被中断。
       然后改成 SA_RESTART，对比效果。
    */
    sa.sa_flags = 0;
    // sa.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("PID = %d\n", getpid());
    printf("Type something, or send: kill -USR1 %d\n", getpid());

    while (1) {
        printf("> ");
        fflush(stdout);

        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);

        if (n == -1) {
            if (errno == EINTR) {
                printf("read interrupted by signal, retrying...\n");
                continue;
            } else {
                perror("read");
                return 1;
            }
        }

        if (n == 0) {
            printf("EOF\n");
            break;
        }

        buf[n] = '\0';
        printf("You typed: %s", buf);
    }

    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g eintr_read.c -o eintr_read
./eintr_read
```

另一个终端：

```bash
kill -USR1 <pid>
```

## 运行逻辑

程序阻塞在：

```c
read(STDIN_FILENO, buf, sizeof(buf) - 1);
```

收到信号后，可能发生：

```c
read() == -1
errno == EINTR
```

所以健壮代码应该写：

```c
if (errno == EINTR) {
    continue;
}
```

再试一次。

## 对比 SA_RESTART

把：

```c
sa.sa_flags = 0;
```

改成：

```c
sa.sa_flags = SA_RESTART;
```

重新编译运行。

这时某些被中断的系统调用会自动重启。

但注意：不是所有系统调用都一定会自动重启，所以生产代码仍然应该理解并处理 `EINTR`。

## 生产中的用处

这是系统编程里非常实际的问题。

生产代码中这些调用都可能遇到中断问题：

1. `read`
2. `write`
3. `accept`
4. `connect`
5. `wait`
6. `recv`
7. `send`
8. `select`
9. `poll`

如果你不处理 `EINTR`，程序可能会把“正常信号打断”误认为“真正错误”，导致服务异常退出。

---

# 练习 13：在信号处理器中使用 sigsetjmp/siglongjmp

## 目标

理解：

* 普通 handler 返回后，程序继续原位置
* `siglongjmp()` 可以从 handler 直接跳回安全点
* 这种做法有风险
* 信号场景更推荐 `sigsetjmp/siglongjmp`

## 任务

程序运行一个死循环，按 Ctrl+C 后不是正常返回，而是直接跳回 main 中保存的位置。

## 示例代码：`sigjump_demo.c`

```c
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

static sigjmp_buf env;

void handler(int sig) {
    (void)sig;
    siglongjmp(env, 1);
}

int main(void) {
    struct sigaction sa;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    if (sigsetjmp(env, 1) == 0) {
        printf("First time through sigsetjmp\n");
        printf("Press Ctrl+C to jump back\n");

        while (1) {
            printf("Working...\n");
            sleep(1);
        }
    } else {
        printf("Returned from signal handler by siglongjmp\n");
    }

    printf("Program exits safely from main\n");

    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g sigjump_demo.c -o sigjump_demo
./sigjump_demo
```

按 Ctrl+C。

## 运行逻辑

第一次：

```c
sigsetjmp(env, 1)
```

返回 `0`，进入工作循环。

按 Ctrl+C 后：

```c
handler()
```

被执行。

handler 中：

```c
siglongjmp(env, 1)
```

直接跳回 `sigsetjmp()` 那里。

这次 `sigsetjmp()` 返回非 0，于是进入：

```c
else {
    printf("Returned from signal handler by siglongjmp\n");
}
```

## 风险

这项技术很强，但危险。

假设信号打断时，主程序正在执行：

```c
malloc()
```

或者正在修改某个复杂数据结构，handler 直接跳走，可能导致内部状态不完整。

## 生产中的用处

少数底层场景会用：

1. 超时恢复；
2. 解释器中断执行；
3. 崩溃恢复框架；
4. 从危险区域跳回安全点；
5. 老式 C 程序模拟异常控制流。

普通服务代码里，优先使用“设置标志位 + 主循环退出”的方式。

---

# 练习 14：abort 和异常终止

## 目标

理解：

* `abort()` 会异常终止进程
* 通常触发 `SIGABRT`
* 可能生成 core dump
* 和 `exit()`、`_exit()` 不同

## 任务

写一个程序，当检测到严重错误时调用 `abort()`。

## 示例代码：`abort_demo.c`

```c
#include <stdio.h>
#include <stdlib.h>

void check_state(int state) {
    if (state != 1) {
        fprintf(stderr, "Fatal error: invalid state = %d\n", state);
        abort();
    }
}

int main(void) {
    printf("Program started\n");

    int state = 0;

    check_state(state);

    printf("This line will not be printed\n");

    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g abort_demo.c -o abort_demo
./abort_demo
```

你可能看到：

```text
Fatal error: invalid state = 0
Aborted
```

## 运行逻辑

`abort()` 表示程序发现了不能恢复的严重错误，主动异常终止。

它和普通：

```c
return 0;
```

不一样。

它也和：

```c
exit(1);
```

不完全一样。

大致区别：

```text
return / exit  ：正常退出流程
_exit          ：直接退出，不做标准清理
abort          ：异常终止，通常用于严重错误和调试
```

## 生产中的用处

`abort()` 常用于：

1. 断言失败；
2. 发现内部状态不可能发生；
3. 调试时故意生成 core dump；
4. 让监控系统发现严重崩溃；
5. 底层库检测到数据结构损坏。

例如：

```c
if (ptr == NULL) {
    abort();
}
```

表示这里理论上绝不应该为空。如果真的为空，继续运行反而更危险。

---

# 练习 15：备用信号栈 sigaltstack

## 目标

理解：

* 信号处理器本身也需要栈
* 普通栈溢出时，handler 可能无法正常运行
* `sigaltstack()` 可以提供备用栈
* `SA_ONSTACK` 指定 handler 在备用栈运行

## 任务

设置一个备用信号栈，并让 `SIGSEGV` 的 handler 在备用栈上执行。

## 示例代码：`altstack_demo.c`

```c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

void segv_handler(int sig) {
    (void)sig;

    const char msg[] = "Caught SIGSEGV on alternate signal stack\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    _exit(1);
}

void overflow_stack(void) {
    char buffer[1024 * 16];

    memset(buffer, 0, sizeof(buffer));

    overflow_stack();
}

int main(void) {
    stack_t ss;
    struct sigaction sa;

    ss.ss_sp = malloc(SIGSTKSZ);
    if (ss.ss_sp == NULL) {
        perror("malloc");
        return 1;
    }

    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;

    if (sigaltstack(&ss, NULL) == -1) {
        perror("sigaltstack");
        free(ss.ss_sp);
        return 1;
    }

    sa.sa_handler = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_ONSTACK;

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        free(ss.ss_sp);
        return 1;
    }

    printf("About to overflow stack...\n");

    overflow_stack();

    free(ss.ss_sp);
    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g altstack_demo.c -o altstack_demo
./altstack_demo
```

## 运行逻辑

程序递归调用：

```c
overflow_stack();
```

每次调用都会消耗栈空间。

最后普通栈溢出，产生 `SIGSEGV`。

因为提前设置了：

```c
sigaltstack(&ss, NULL);
```

并且：

```c
sa.sa_flags = SA_ONSTACK;
```

所以 `SIGSEGV` 的处理器会在备用信号栈上运行。

## 为什么 handler 里用 `_exit()`？

这里已经发生严重错误，进程状态不可靠。

所以 handler 中直接调用：

```c
_exit(1);
```

比 `exit(1)` 更合适，因为 `exit()` 会执行标准清理流程，可能调用不安全逻辑。

## 生产中的用处

普通业务开发很少直接用 `sigaltstack()`。

但它在这些领域很重要：

1. 崩溃处理器；
2. profiler；
3. 调试器；
4. 语言运行时；
5. 虚拟机；
6. 高可靠服务的崩溃诊断；
7. 捕获栈溢出并打印诊断信息。

---

# 练习 16：综合项目：mini_worker

## 目标

把三章连起来：

* PID/PPID
* argc/argv
* getenv
* sigaction
* `volatile sig_atomic_t`
* SIGINT/SIGTERM
* SIGUSR1
* SA_SIGINFO
* 优雅退出
* 简单生产风格日志

## 任务

写一个 worker：

```bash
./mini_worker 3
```

含义：

每 3 秒处理一次任务。

环境变量：

```bash
WORKER_NAME=alpha ./mini_worker 3
```

程序功能：

1. 打印 PID、PPID；
2. 读取命令行参数作为工作间隔；
3. 读取环境变量 `WORKER_NAME`；
4. 捕获 `SIGINT` 和 `SIGTERM`，优雅退出；
5. 捕获 `SIGUSR1`，打印状态；
6. handler 只设置标志；
7. 主循环执行真正逻辑。

## 示例代码：`mini_worker.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t stop = 0;
static volatile sig_atomic_t show_status = 0;
static volatile sig_atomic_t usr1_sender_pid = 0;

void stop_handler(int sig) {
    (void)sig;
    stop = 1;
}

void usr1_handler(int sig, siginfo_t *info, void *ucontext) {
    (void)sig;
    (void)ucontext;

    show_status = 1;
    usr1_sender_pid = info->si_pid;
}

int main(int argc, char *argv[]) {
    int interval = 1;

    if (argc >= 2) {
        interval = atoi(argv[1]);
        if (interval <= 0) {
            fprintf(stderr, "Invalid interval: %s\n", argv[1]);
            return 1;
        }
    }

    char *worker_name = getenv("WORKER_NAME");
    if (worker_name == NULL) {
        worker_name = "default-worker";
    }

    struct sigaction sa_stop;
    sa_stop.sa_handler = stop_handler;
    sigemptyset(&sa_stop.sa_mask);
    sa_stop.sa_flags = 0;

    if (sigaction(SIGINT, &sa_stop, NULL) == -1) {
        perror("sigaction SIGINT");
        return 1;
    }

    if (sigaction(SIGTERM, &sa_stop, NULL) == -1) {
        perror("sigaction SIGTERM");
        return 1;
    }

    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = usr1_handler;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("sigaction SIGUSR1");
        return 1;
    }

    printf("Worker name = %s\n", worker_name);
    printf("PID         = %d\n", getpid());
    printf("PPID        = %d\n", getppid());
    printf("Interval    = %d second(s)\n", interval);
    printf("Send SIGUSR1: kill -USR1 %d\n", getpid());
    printf("Stop: Ctrl+C or kill -TERM %d\n", getpid());

    int job_count = 0;

    while (!stop) {
        job_count++;

        printf("[%s] processing job %d\n", worker_name, job_count);

        for (int i = 0; i < interval; i++) {
            if (stop || show_status) {
                break;
            }
            sleep(1);
        }

        if (show_status) {
            printf("[%s] status requested by PID %d\n",
                   worker_name, usr1_sender_pid);
            printf("[%s] jobs processed = %d\n",
                   worker_name, job_count);

            show_status = 0;
        }
    }

    printf("[%s] stopping gracefully...\n", worker_name);
    printf("[%s] total jobs processed = %d\n", worker_name, job_count);
    printf("[%s] bye\n", worker_name);

    return 0;
}
```

运行：

```bash
gcc -Wall -Wextra -O0 -g mini_worker.c -o mini_worker
WORKER_NAME=alpha ./mini_worker 3
```

另一个终端：

```bash
kill -USR1 <pid>
```

停止：

```bash
kill -TERM <pid>
```

或者按：

```text
Ctrl+C
```

## 这个综合项目串起来的知识点

### 1. 进程身份

```c
getpid()
getppid()
```

用于知道当前 worker 的进程编号和父进程。

### 2. 命令行参数

```c
argc
argv
atoi(argv[1])
```

让用户从外部控制工作间隔。

### 3. 环境变量

```c
getenv("WORKER_NAME")
```

让部署环境控制 worker 名称。

### 4. 信号处理

```c
sigaction(SIGINT, ...)
sigaction(SIGTERM, ...)
sigaction(SIGUSR1, ...)
```

让程序响应外部控制。

### 5. 安全 handler

handler 中只写：

```c
stop = 1;
show_status = 1;
```

复杂的 `printf()` 放回主循环执行。

### 6. SA_SIGINFO

```c
info->si_pid
```

可以知道是谁发来了 `SIGUSR1`。

## 生产中的对应场景

这个 `mini_worker` 很像真实后端服务：

```text
启动服务
读取配置
打印 PID
处理任务
收到 SIGUSR1 打印状态
收到 SIGTERM 优雅退出
```

真实系统里可以扩展成：

1. 后台任务 worker；
2. 消息队列消费者；
3. 日志处理进程；
4. 定时任务进程；
5. 守护进程；
6. 容器中的主进程。

---

# 建议你的练习顺序

推荐你这样做：

```text
第 1 天：
练习 1、2、3、4
重点：进程、内存、栈、参数、环境变量

第 2 天：
练习 5、6、7、8
重点：setjmp/longjmp、signal、kill、sigaction

第 3 天：
练习 9、10、11、12
重点：信号掩码、pending、pause、SA_SIGINFO、EINTR

第 4 天：
练习 13、14、15、16
重点：高级信号处理、异常终止、备用栈、综合项目
```

最重要的是：不要只复制代码运行。每个练习至少改一次，比如：

```text
改信号类型
改循环条件
改环境变量名
改命令行参数
改 handler 行为
改 SA_RESTART
多开几个终端发送信号
```

这样你才能真的把“看懂知识点”变成“会在代码里使用”。