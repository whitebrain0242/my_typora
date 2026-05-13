# MyLs

可以。这个任务很适合把你前面学过的知识真正串起来：**目录读取、文件属性、inode、权限、时间、递归、排序、参数解析、内存管理**都会用到。

你不要一开始就想着“我要写完整 ls”。正确思路是：

```text
先写一个只能列当前目录的 myls
再支持路径参数
再支持 -a
再支持 -l
再支持排序
再支持递归
最后美化、组合参数、检查泄漏
```

下面我帮你拆成 **4 天计划**。每天都有：目标、要实现的功能、需要用到的知识、建议步骤、测试方式、博客记录点。

------

# 总体目标

最终你要写出一个程序，比如叫：

```bash
myls
```

支持：

```bash
./myls
./myls -a
./myls -l
./myls -al
./myls -a -l
./myls -ai -t /home
./myls -i /home /etc
./myls -alR /
```

需要支持的参数：

| 参数 | 功能                           |
| ---- | ------------------------------ |
| `-a` | 显示隐藏文件，包括 `.` 和 `..` |
| `-l` | 长格式显示                     |
| `-R` | 递归显示目录                   |
| `-t` | 按修改时间排序                 |
| `-r` | 反向排序                       |
| `-i` | 显示 inode 号                  |
| `-s` | 显示文件占用块数               |

------

# 建议项目结构

一开始可以简单一点，后面再拆文件。

第 1 天可以先只写一个文件：

```text
myls.c
```

第 3、4 天可以拆成：

```text
myls/
├── main.c          // main 函数、参数解析
├── ls.h           // 结构体和函数声明
├── list.c         // 目录读取、文件信息收集
├── print.c        // 输出显示
├── sort.c         // 排序
├── utils.c        // 权限字符串、时间格式、路径拼接等工具
└── Makefile
```

不过你刚开始不要急着拆。建议：

```text
先让功能跑通，再重构拆文件。
```

------

# 第 1 天：写出最简单版本的 ls

## 今日目标

写出一个最基础的版本：

```bash
./myls
```

可以显示当前目录下的普通文件名。

再进一步支持：

```bash
./myls /home
./myls /etc
./myls /home /etc
```

也就是先实现：

```text
读取目录
遍历目录项
输出文件名
支持一个或多个路径参数
```

------

## 今天先不做什么？

今天先不要做：

```text
-a
-l
-R
-t
-r
-i
-s
颜色
对齐
```

第一天只关心一件事：

>  我能不能打开一个目录，并把目录里的文件名读出来。

------

## 需要用到的知识

### 1. 命令行参数

你要理解：

```c
int main(int argc, char *argv[])
```

例如用户输入：

```bash
./myls /home /etc
```

那么大概是：

```text
argc = 3

argv[0] = "./myls"
argv[1] = "/home"
argv[2] = "/etc"
```

所以你要学会从 `argv` 里取路径。

------

### 2. 目录操作函数

今天最核心的是这三个：

```c
opendir()
readdir()
closedir()
```

它们的关系类似：

```text
opendir   打开目录
readdir   一个一个读取目录项
closedir  关闭目录
```

可以类比你之前学的：

```text
open      打开文件
read      读取文件内容
close     关闭文件
```

只不过目录不能用普通 `read()` 直接舒服地读，所以 Linux 提供了目录专用函数。

------

## 今天要实现的功能板块

### 板块 1：默认列出当前目录

用户输入：

```bash
./myls
```

你应该默认显示：

```text
当前目录 .
```

也就是说，没传路径时，相当于：

```bash
./myls .
```

------

### 板块 2：列出指定目录

用户输入：

```bash
./myls /home
```

你要打开 `/home`，然后显示里面的文件名。

------

### 板块 3：支持多个路径

用户输入：

```bash
./myls /home /etc
```

你可以输出成类似：

```text
/home:
user1 user2

/etc:
passwd hosts profile ...
```

只要多个路径时加一个目录标题即可。

------

### 板块 4：错误处理

如果用户输入：

```bash
./myls /no/such/dir
```

不要崩溃，要输出错误：

```text
myls: cannot open '/no/such/dir': No such file or directory
```

你需要用：

```c
perror()
```

或者：

```c
strerror(errno)
```

------

## 第 1 天你应该理解的问题

今天结束时，你要能回答这几个问题：

```text
1. argc 和 argv 是什么？
2. opendir() 成功返回什么？失败返回什么？
3. readdir() 每次返回什么？
4. 为什么用完目录后一定要 closedir()？
5. 为什么默认路径应该是 "."？
```

------

## 第 1 天测试命令

你可以这样测试：

```bash
gcc myls.c -o myls
./myls
./myls .
./myls /home
./myls /etc
./myls /home /etc
./myls /no/such/dir
```

然后对比系统的：

```bash
ls
ls /home
ls /etc
```

注意：第一天你的输出不用和系统 `ls` 一模一样，只要能列文件名就行。

------

## 第 1 天博客记录

今天博客可以写：

```text
1. 我为什么不能直接用 read() 读目录？
2. opendir/readdir/closedir 的使用流程
3. argc/argv 如何接收命令行参数
4. 我遇到的第一个错误是什么，怎么解决的
```

------

# 第 2 天：实现 -a、-i、-s，并建立文件信息结构体

## 今日目标

今天开始让 `myls` 有参数功能。

先实现这三个：

```text
-a    显示隐藏文件
-i    显示 inode
-s    显示磁盘块数
```

这三个相对简单，适合第二天完成。

------

## 今天要新增的核心思想

第 1 天你只是边读边输出：

```text
读到一个文件名，马上 printf
```

但从今天开始，不建议这样做。

你应该先把每个文件的信息保存起来，比如：

```text
文件名
完整路径
stat 信息
```

以后 `-l`、`-t`、`-s`、`-i` 都要依赖这些信息。

所以你需要设计一个结构体。

可以先这样想，不急着直接抄代码：

```c
struct file_info {
    char *name;          // 文件名
    char *path;          // 完整路径
    struct stat st;      // 文件属性
};
```

这个结构体非常重要。

你可以理解为：

```text
每一个目录项，都被我整理成一个 file_info 对象。
```

------

## 需要用到的知识

### 1. `stat()` / `lstat()`

这两个函数用来获取文件属性。

你需要的信息包括：

```text
inode        st_ino
文件大小     st_size
权限         st_mode
链接数       st_nlink
uid          st_uid
gid          st_gid
修改时间     st_mtime
占用块数     st_blocks
```

建议你用：

```c
lstat()
```

而不是 `stat()`。

原因是：

```text
lstat() 遇到符号链接时，查看的是链接本身
stat() 遇到符号链接时，会跟随链接查看目标文件
```

写 `ls -l` 时，通常需要知道符号链接本身，所以用 `lstat()` 更合适。

------

### 2. 隐藏文件判断

Linux 里隐藏文件的规则很简单：

```text
文件名以 . 开头，就是隐藏文件
```

比如：

```text
.bashrc
.profile
.git
```

所以没有 `-a` 时，你跳过：

```text
name[0] == '.'
```

有 `-a` 时才显示它们。

------

### 3. inode

`-i` 要显示 inode 号。

inode 来自：

```c
st.st_ino
```

你之前学过 inode，可以这样理解：

```text
文件名只是目录里的名字
inode 才是文件真正的身份编号
```

所以：

```bash
ls -i
```

显示的是每个文件对应的 inode 号。

------

### 4. 块数

`-s` 显示文件占用的磁盘块数。

要用：

```c
st.st_blocks
```

注意：

```text
st_blocks 通常以 512 字节为单位
GNU ls -s 默认显示 1K blocks
```

所以你可以先简单处理为：

```text
显示 st_blocks / 2
```

初学阶段这样够用。

------

## 今天要实现的功能板块

### 板块 1：参数解析

你需要能识别：

```bash
./myls -a
./myls -i
./myls -s
./myls -ais
./myls -a -i -s
```

重点是：

```text
-a -i -s
```

和：

```text
-ais
```

都要能识别。

建议定义一个选项结构体：

```c
struct options {
    int show_all;      // -a
    int show_inode;    // -i
    int show_blocks;   // -s
    int long_format;   // -l，今天先预留
    int recursive;     // -R，今天先预留
    int sort_time;     // -t，今天先预留
    int reverse;       // -r，今天先预留
};
```

今天只真正使用：

```text
show_all
show_inode
show_blocks
```

但是提前预留后面的选项，会让后面更顺。

------

### 板块 2：读取目录时保存 file_info

从今天开始，流程变成：

```text
打开目录
循环 readdir
判断是否跳过隐藏文件
拼接完整路径
lstat 获取属性
保存到数组
最后统一输出
释放内存
关闭目录
```

这里你会第一次接触比较明显的内存管理。

------

### 板块 3：实现 -a

测试：

```bash
./myls
./myls -a
```

你应该发现：

```text
不加 -a：不显示 . 开头的文件
加 -a：显示 .、..、.bashrc、.git 等
```

------

### 板块 4：实现 -i

测试：

```bash
./myls -i
```

输出类似：

```text
123456 main.c
123457 myls.c
123458 Makefile
```

------

### 板块 5：实现 -s

测试：

```bash
./myls -s
```

输出类似：

```text
4 main.c
8 myls.c
4 Makefile
```

如果同时有：

```bash
./myls -is
```

可以输出：

```text
123456 4 main.c
```

顺序可以先定为：

```text
inode  block  name
```

------

## 第 2 天你应该理解的问题

今天结束时，你要能回答：

```text
1. stat 和 lstat 有什么区别？
2. struct stat 里面有哪些重要字段？
3. 隐藏文件为什么只是以 . 开头？
4. inode 为什么不是文件名？
5. st_size 和 st_blocks 有什么区别？
6. 为什么要先保存文件信息，而不是读一个打印一个？
```

其中第 6 个很重要。

因为后面 `-t` 排序必须先保存起来再排序，否则你一边读一边打印，就没法排序了。

------

## 第 2 天测试命令

```bash
./myls
./myls -a
./myls -i
./myls -s
./myls -is
./myls -ais
./myls -a -i -s
./myls -i /home /etc
```

对比：

```bash
ls -a
ls -i
ls -s
ls -ais
```

------

## 第 2 天博客记录

可以写：

```text
1. 我为什么设计 file_info 结构体
2. stat/lstat 的区别
3. inode 是什么
4. st_size 和 st_blocks 的区别
5. 参数解析时如何支持 -a -i 和 -ai
```

------

# 第 3 天：实现 -l、-t、-r，输出开始接近真正 ls

## 今日目标

今天是整个任务中最关键的一天。

要实现：

```text
-l    长格式显示
-t    按时间排序
-r    反向排序
```

这一天会让你的程序从“能列文件名”变成“像一个真正的 ls”。

------

# 一、实现 -l

## `ls -l` 要显示什么？

系统的：

```bash
ls -l
```

大概是：

```text
-rw-r--r-- 1 user group 1234 May 12 20:30 main.c
drwxr-xr-x 2 user group 4096 May 12 20:31 src
lrwxrwxrwx 1 user group    5 May 12 20:32 link -> a.txt
```

你需要输出这些信息：

```text
文件类型 + 权限
硬链接数
用户名
组名
文件大小
修改时间
文件名
```

对应 `struct stat`：

| 显示内容 | 字段       |
| -------- | ---------- |
| 文件类型 | `st_mode`  |
| 权限     | `st_mode`  |
| 链接数   | `st_nlink` |
| 用户 ID  | `st_uid`   |
| 组 ID    | `st_gid`   |
| 文件大小 | `st_size`  |
| 修改时间 | `st_mtime` |
| 文件名   | `name`     |

------

## 需要用到的知识

### 1. 文件类型判断

通过 `st_mode` 判断：

```c
S_ISREG(st_mode)   普通文件
S_ISDIR(st_mode)   目录
S_ISLNK(st_mode)   符号链接
S_ISCHR(st_mode)   字符设备
S_ISBLK(st_mode)   块设备
S_ISFIFO(st_mode)  管道
S_ISSOCK(st_mode)  socket
```

显示第一个字符：

```text
-    普通文件
d    目录
l    符号链接
c    字符设备
b    块设备
p    管道
s    socket
```

------

### 2. 权限字符串

你需要把 `st_mode` 转成：

```text
rwxr-xr-x
rw-r--r--
```

最终是 10 个字符：

```text
-rw-r--r--
drwxr-xr-x
lrwxrwxrwx
```

第 1 个是文件类型，后 9 个是权限：

```text
用户权限  组权限  其他人权限
rwx       r-x     r-x
```

------

### 3. UID/GID 转用户名和组名

`st_uid` 是数字，比如：

```text
1000
```

但 `ls -l` 显示用户名，比如：

```text
signe
```

要用：

```c
getpwuid()
getgrgid()
```

头文件：

```c
#include <pwd.h>
#include <grp.h>
```

如果转换失败，就显示数字 ID。

------

### 4. 时间格式

`st_mtime` 是时间戳，不适合直接打印。

你需要用：

```c
localtime()
strftime()
```

先做简单版本即可，比如：

```text
2026-05-12 20:30
```

不一定要完全模仿 GNU `ls` 的月份格式。

------

### 5. 符号链接显示

如果文件是符号链接，`ls -l` 通常显示：

```text
link -> target
```

你需要用：

```c
readlink()
```

注意：

```text
readlink() 不会自动给字符串加 '\0'
```

所以你要自己补：

```c
buf[n] = '\0';
```

------

# 二、实现 -t

## `-t` 的含义

```bash
ls -t
```

表示：

```text
按照修改时间排序，新的在前
```

也就是比较：

```c
st.st_mtime
```

你的程序之前已经把所有文件保存到数组里了，所以现在可以用：

```c
qsort()
```

排序。

------

## 排序规则建议

默认排序：

```text
按文件名升序
```

加 `-t`：

```text
按修改时间降序，也就是新的在前
```

如果时间相同：

```text
再按文件名排序
```

这样输出比较稳定。

------

# 三、实现 -r

## `-r` 的含义

```bash
ls -r
```

表示：

```text
反向排序
```

所以：

```bash
ls
```

是：

```text
a b c
```

那么：

```bash
ls -r
```

是：

```text
c b a
```

如果：

```bash
ls -t
```

是：

```text
new old
```

那么：

```bash
ls -tr
```

就是：

```text
old new
```

你可以有两种实现方式：

```text
方式 1：比较函数里判断 reverse
方式 2：先正常排序，输出时倒着输出
```

初学建议用方式 2：

```text
排序逻辑简单
输出时根据 -r 决定从前往后还是从后往前
```

------

## 第 3 天功能板块

### 板块 1：实现权限字符串函数

你可以设计一个函数：

```text
mode_to_string()
```

功能：

```text
输入 st_mode
输出类似 -rw-r--r--
```

------

### 板块 2：实现用户名、组名转换

实现：

```text
uid_to_name()
gid_to_name()
```

如果找不到名字，就显示数字。

------

### 板块 3：实现时间格式化

实现：

```text
format_time()
```

先输出：

```text
YYYY-MM-DD HH:MM
```

------

### 板块 4：实现长格式打印

普通输出：

```bash
./myls
```

只显示文件名。

长格式输出：

```bash
./myls -l
```

显示完整信息。

------

### 板块 5：实现排序

先默认按名字排序：

```bash
./myls
```

然后实现：

```bash
./myls -t
./myls -r
./myls -tr
./myls -rt
```

------

## 第 3 天测试命令

```bash
./myls -l
./myls -al
./myls -li
./myls -ls
./myls -t
./myls -r
./myls -tr
./myls -ltr
./myls -altr /etc
```

对比系统：

```bash
ls -l
ls -al
ls -t
ls -r
ls -tr
ls -altr /etc
```

------

## 第 3 天你应该理解的问题

```text
1. st_mode 里为什么既有文件类型又有权限？
2. -rw-r--r-- 每一位分别表示什么？
3. st_uid 怎么转成用户名？
4. st_mtime 怎么转成人能看懂的时间？
5. 为什么排序前必须把目录项都保存到数组里？
6. -t 和 -r 组合时应该怎么处理？
```

------

## 第 3 天博客记录

可以写：

```text
1. ls -l 每一列的来源
2. st_mode 如何解析文件类型和权限
3. readlink() 的坑：不会自动补 '\0'
4. qsort() 如何实现 -t 排序
5. -r 是如何和 -t 组合的
```

------

# 第 4 天：实现 -R、颜色、对齐、内存泄漏检查和最终整理

## 今日目标

最后一天做完整性和质量。

实现：

```text
-R    递归显示目录
颜色显示
输出对齐
参数任意组合
无资源与内存泄漏
博客总结
上传仓库
```

------

# 一、实现 -R

## `-R` 的含义

```bash
ls -R
```

表示：

```text
列出当前目录
再进入当前目录里的子目录
再列出子目录
一直递归下去
```

例如：

```text
.:
a.txt src

./src:
main.c utils.c
```

------

## 递归的核心逻辑

你可以把它理解成：

```text
list_dir(path):
    1. 显示 path 这个目录下的内容
    2. 如果开启 -R：
        遍历刚才读到的文件
        找出其中的目录
        跳过 . 和 ..
        对每个子目录再次调用 list_dir()
```

这就是递归。

------

## `-R` 的关键注意点

### 1. 一定要跳过 `.` 和 `..`

否则会无限递归：

```text
.  表示当前目录
.. 表示上一级目录
```

如果你递归进入 `.`，就会永远进入自己。

如果你递归进入 `..`，就会一路往上跑，逻辑混乱。

所以必须跳过：

```text
.
..
```

------

### 2. 不建议递归进入符号链接目录

如果一个符号链接指向目录，系统 `ls -R` 的行为有一些细节。

你初学阶段建议：

```text
只递归真正的目录
不递归符号链接目录
```

判断时使用：

```c
S_ISDIR(st.st_mode)
```

因为你用的是 `lstat()`，符号链接不会被当成真实目录。

------

### 3. 遇到无权限目录不能崩溃

特别是测试：

```bash
./myls -alR /
```

你会遇到很多目录不能访问。

例如：

```text
/proc/xxx
/root
/sys/...
```

你的程序应该：

```text
打印错误
继续处理其他目录
```

不能因为一个目录打不开就整个退出。

------

### 4. `/` 遍历测试会很大

验收要求提到：

```text
-R 需要对 / 的遍历测试
```

这说明你的递归逻辑必须稳定。

但是你自己调试时不要一上来就：

```bash
./myls -alR /
```

建议按顺序测试：

```bash
./myls -R testdir
./myls -R /tmp
./myls -R /etc
./myls -R /
```

------

# 二、颜色显示

## 颜色的基本原理

终端颜色通常用 ANSI 转义序列。

比如：

```text
蓝色目录
绿色可执行文件
青色符号链接
红色错误或坏链接
```

你可以先做简单版：

| 类型       | 颜色   |
| ---------- | ------ |
| 目录       | 蓝色   |
| 可执行文件 | 绿色   |
| 符号链接   | 青色   |
| 普通文件   | 默认色 |

你可以设计：

```text
print_name_with_color(file_info)
```

它根据文件类型决定颜色。

------

## 注意

如果输出被重定向到文件，比如：

```bash
./myls > result.txt
```

严格来说不应该输出颜色码。

这个可以进阶处理：

```c
isatty(STDOUT_FILENO)
```

如果是终端才显示颜色。

初学版可以先不做，最后有时间再加。

------

# 三、输出对齐

## 为什么要对齐？

如果你直接打印：

```text
1 a
123456 longfilename
33 file
```

会比较乱。

你可以先统计最大宽度：

```text
最大 inode 位数
最大 block 位数
最大 link 位数
最大 size 位数
最大用户名长度
最大组名长度
```

然后用 `printf` 的宽度控制：

```c
printf("%*lu", width, value);
```

比如：

```c
printf("%8lu", inode);
```

表示宽度至少 8，不够左边补空格。

------

## 对齐建议

普通模式下：

```text
inode blocks name
```

长格式下：

```text
inode blocks permissions links user group size time name
```

你不需要和 GNU `ls` 一模一样，但要做到：

```text
整齐
可读
不同文件长度不会乱成一团
```

------

# 四、无资源与内存泄漏

这是验收要求之一，非常重要。

你需要检查：

```text
1. opendir() 后是否一定 closedir()
2. malloc() 后是否一定 free()
3. strdup() 后是否一定 free()
4. 出错路径是否也释放资源
5. 递归过程中是否释放每一层目录的数据
```

------

## 推荐使用 valgrind

编译时加调试信息：

```bash
gcc -g -Wall -Wextra myls.c -o myls
```

检查：

```bash
valgrind --leak-check=full ./myls -alR testdir
```

你希望看到类似：

```text
definitely lost: 0 bytes
indirectly lost: 0 bytes
```

------

# 五、最终功能组合测试

你需要测试这些：

```bash
./myls
./myls -a
./myls -l
./myls -R
./myls -t
./myls -r
./myls -i
./myls -s

./myls -al
./myls -alR
./myls -tr
./myls -ais
./myls -ai -t /home
./myls -i /home /etc
./myls -alR /
```

尤其是任务给的示例：

```bash
./myls -a -l
./myls -a /home
./myls -alR /
./myls -t
./myls -ai -t /home
./myls -i /home /etc
```

------

# 4 天总路线图

你可以按这个版本推进：

```text
Day 1：myls v0.1
只实现 ./myls、./myls /path
能读目录，能显示文件名。

Day 2：myls v0.2
实现参数解析、-a、-i、-s。
建立 file_info 结构体，为后面 -l 和排序做准备。

Day 3：myls v0.3
实现 -l、-t、-r。
能显示权限、用户、组、大小、时间，能排序。

Day 4：myls v1.0
实现 -R、颜色、对齐、错误处理、内存检查、博客总结、上传仓库。
```

------

# 我建议你的核心函数设计

先不用急着写完整代码，但你可以按这些函数去组织思路：

```text
parse_options()
解析 -a -l -R -t -r -i -s

list_path()
处理一个路径，判断它是文件还是目录

list_dir()
列出一个目录内容

read_dir_entries()
读取目录并保存 file_info 数组

sort_entries()
根据 -t、-r 排序

print_entries()
根据选项输出文件列表

print_long()
输出 -l 格式

mode_to_string()
把 st_mode 转成 -rw-r--r--

format_time()
把 st_mtime 转成可读时间

join_path()
把目录路径和文件名拼成完整路径

free_entries()
释放 file_info 数组
```

你最终的主流程应该像这样：

```text
main
 |
 |-- parse_options
 |
 |-- 如果没有路径，默认使用 "."
 |
 |-- 对每个路径：
       |
       |-- list_path(path, options)
```

`list_path()` 再决定：

```text
如果 path 是普通文件：
    直接显示这个文件

如果 path 是目录：
    调用 list_dir()
```

------

# 最重要的学习主线

你这次不是单纯完成作业，而是要通过这个项目把知识串起来。

这个 `ls` 项目对应的知识关系是：

```text
命令行参数
    ↓
argc / argv

读取目录
    ↓
opendir / readdir / closedir

获取文件属性
    ↓
lstat / struct stat

inode
    ↓
st_ino

文件大小和块数
    ↓
st_size / st_blocks

权限和文件类型
    ↓
st_mode / S_ISDIR / S_ISREG / S_IRUSR

用户名和组名
    ↓
getpwuid / getgrgid

时间显示
    ↓
st_mtime / localtime / strftime

排序
    ↓
qsort

递归
    ↓
函数自己调用自己

内存管理
    ↓
malloc / realloc / strdup / free

资源管理
    ↓
closedir / free / valgrind
```

------

# 每天学习时的正确节奏

你每天可以按这个节奏来：

```text
1. 先明确今天只做哪些功能
2. 画出流程图
3. 写最小代码
4. 编译
5. 用简单目录测试
6. 和系统 ls 对比
7. 记录 bug
8. 写博客总结
```

不要一边想 `-l`，一边想 `-R`，一边又想颜色。

这样很容易乱。

你要坚持一个原则：

>  每次只让程序多会一点点。

------

# 第一天你现在就可以开始的任务

你今天先只做这个：

```text
任务 1：写出 ./myls，默认列出当前目录
任务 2：支持 ./myls /home
任务 3：支持 ./myls /home /etc
任务 4：遇到打不开的目录时打印错误，不崩溃
```

第一天完成后，你的程序即使很丑也没关系。

第一天的验收标准只有一个：

```text
我能用 opendir/readdir/closedir 把目录里的文件名列出来。
```

下一步我们就可以从 **第 1 天第 1 小节：main(argc, argv) 和 opendir/readdir 的流程图** 开始，我一步一步带你写。