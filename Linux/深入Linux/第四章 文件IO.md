## 第四章 文件I/O

### 4.1 概述

>  一切皆文件

fd：file descriptor，文件描述符，就是一个整数，是一个编号，一般从3开始

为什么是从3开始，因为0一般是键盘输入，1是屏幕输出，2是错误输出

`fd 0  -> 键盘
     fd 1  -> 屏幕
     fd 2  -> 错误输出
     fd 3  -> 你打开的 a.txt`

### 4.2 通用I/O

为什么是通用？

因为根本不在乎你处理的是什么，它可以处理很多

```c
普通文件
终端
管道
Socket
设备文件
```

你只需要处理接口，但是细节交给内核就好了

### 4.3open()

打开一个文件

```c
int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);

int fd = open("hello.txt", O_RDONLY);
```

成功-》返回文件描述符

失败-》返回-1

**养成习惯：检查返回值**

```c
if (fd == -1) {
    perror("open");
}
```

#### 4.3.1flags

`flags` 决定你怎么打开文件。

**必须指定的访问模式**：

三选一

```c
O_RDONLY    只读
O_WRONLY    只写
O_RDWR      可读可写
```

**其他**：

```c
O_CREAT：文件不存在就创建
O_TRUNC：打开时清空文件内容
O_APPEND：追加写
O_EXCL：配合 O_CREAT，防止覆盖已有文件
```



组合方式：用|连接

权限mode：

```c
4 = 读 r
2 = 写 w
1 = 执行 x
文件所有者：可读可写
同组用户：可读
其他用户：可读
```

**umask**

为什么创建出来的权限可能不是输入的？

```
最终权限 = mode & ~umask
最终权限要看你申请的权限加上收umask的影响的
```

#### 4.3.2open()函数的错误

```c
ENOENT      文件不存在
EACCES      权限不够
EISDIR      试图把目录当普通文件写
EMFILE      当前进程打开文件太多
ENFILE      系统打开文件太多
EEXIST      配合 O_CREAT | O_EXCL 时，文件已存在
```

#### 4.3.3 creat()调用

```c
int creat(const char *pathname, mode_t mode);
等价于
open(pathname, O_WRONLY | O_CREAT | O_TRUNC, mode);
```

现代编程更适合open()

### 4.4 read()

```c
fd       要读的文件描述符
buf      读到的数据放在哪里
count    最多读取多少字节
```



```c
ssize_t read(int fd, void *buf, size_t count);
```

read的返回值是**实际读到的字节数**，所以说read不一定读满你要求的字节数，为什么呢？

可能是因为文件的剩余内容不够了，或者终端只输入了一行等

如果=0，那就是读到了文件的末尾

如果=-1，那就是读取失败

### 4.5 write()

```c
ssize_t write(int fd, const void *buf, size_t count);
```

```c
fd       写到哪个文件
buf      要写的数据在哪里
count    希望写多少字节
```

返回值：如果大于等于0，就是实际写入的字节数

​               如果=-1，就是写入失败

所以说：write也不一定能够写满要求的字节数，没有写满就是**部分写入**

为什么呢？可能因为磁盘的空间不够了，管道缓冲区不够

### 4.6close()

告诉内核：这个文件我不用了。

```c
int close(int fd);
```

返回值：如果是0就是成功，如果是-1就是失败

为什么要close？

```c
文件描述符泄漏
进程打开文件数量越来越多
可能导致 open() 失败
数据可能没有及时写入
资源长期占用
```

### 4.7lseek()

改变文件偏移量，就像是你读一本书，你可以用它来跳到你已经阅读过的部分，下一次嗲用read或者是write就是上次的位置接着继续

```c
回到文件开头
lseek(fd, 0, SEEK_SET);

跳过前 100 个字节
lseek(fd, 100, SEEK_SET);

从当前位置往后移动 10 字节
lseek(fd, 10, SEEK_CUR);

移动到文件末尾
lseek(fd, 0, SEEK_END);

4. 获取文件大小
off_t size = lseek(fd, 0, SEEK_END);


这会把文件偏移量移动到末尾，并返回末尾位置。

因为文件末尾的位置通常就是文件大小。
```

对了，lseek不能用于所有的fd

```c
管道
Socket
终端
FIFO
```

为什么？因为你你可以把这些当作是水流，你只能顺序下去，但是不能倒着上去



**文件空洞**：是什么？大概就是前10个写了，用lseek跳到第100个，继续写，导致中间有一部分是空的，什么也没有

看起来很大，但实际内容却很少，中间的部分，读取的时候大概是\0

### 4.8ioctl

input output control它用于执行一些普通 `read()`、`write()` 做不了的控制操作

```c
获取终端窗口大小
控制设备参数
设置网卡参数
控制磁盘设备
控制串口
控制驱动程序
```



### 练习

1.cat

```c
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int fd = open("a.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    char buf[1024];
    ssize_t n;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, n);
    }

    if (n == -1) {
        perror("read");
    }

    close(fd);
    return 0;
}
```

2.cp

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int src = open("source.txt", O_RDONLY);
    if (src == -1) {
        perror("open source");
        return 1;
    }

    int dst = open("dest.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst == -1) {
        perror("open dest");
        close(src);
        return 1;
    }

    char buf[4096];
    ssize_t n;

    while ((n = read(src, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;

        while (written < n) {
            ssize_t m = write(dst, buf + written, n - written);
            if (m == -1) {
                perror("write");
                close(src);
                close(dst);
                return 1;
            }
            written += m;
        }
    }

    if (n == -1) {
        perror("read");
    }

    close(src);
    close(dst);

    return 0;
}

```

