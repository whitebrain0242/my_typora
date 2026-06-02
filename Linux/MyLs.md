可以。这个任务我们要按“**项目拆解 + 知识点理解 + 分版本实现**”的方式来做，而不是一上来直接写一大坨代码。

你要实现的不是完整 GNU `ls`，而是一个支持这些参数的简化版：

```bash
-a  显示隐藏文件
-l  长格式显示
-R  递归显示目录
-t  按修改时间排序
-r  反向排序
-i  显示 inode
-s  显示占用块数
```

我建议你的程序叫：

```bash
myls
```

使用方式类似：

```bash
./myls -a -l
./myls -a /home
./myls -alR /
./myls -t
./myls -ai -t /home
./myls -i /home /etc
```

------

# 一、这个 `ls` 项目本质上在做什么？

`ls` 的核心不是简单地 `printf` 文件名。

它真正做的是：

```text
读取目录内容
    ↓
获取每个文件的属性
    ↓
根据参数决定是否显示、如何排序
    ↓
根据参数决定输出格式
    ↓
如果有 -R，递归进入子目录
```

所以你自己的 `myls` 可以理解为：

> 一个“目录信息收集器 + 文件属性分析器 + 格式化输出器”。

------

# 二、我把整个任务分成 9 个大板块

整体分成：

```text
板块 1：命令行参数解析
板块 2：路径识别与处理
板块 3：目录读取
板块 4：文件属性获取
板块 5：普通格式输出
板块 6：-l 长格式输出
板块 7：排序功能 -t 和 -r
板块 8：递归功能 -R
板块 9：美化、对齐、颜色、资源释放
```

程序整体流程大概是：

```text
main()
  |
  |-- 解析参数
  |
  |-- 找到路径参数
  |
  |-- 如果没有路径，默认使用 "."
  |
  |-- 对每个路径：
        |
        |-- 判断它是文件还是目录
        |
        |-- 如果是普通文件，直接显示
        |
        |-- 如果是目录：
              |
              |-- 读取目录内容
              |-- 获取每个文件属性
              |-- 排序
              |-- 输出
              |-- 如果有 -R，继续递归子目录
              |-- 释放资源
```

------

# 三、板块 1：命令行参数解析

## 1. 这个板块要解决什么问题？

用户可能这样输入：

```bash
./myls -a -l
./myls -alR /
./myls -ai -t /home
./myls -i /home /etc
```

你的程序要能分清楚：

```text
哪些是选项？
哪些是路径？
```

例如：

```bash
./myls -ai -t /home
```

应该解析成：

```text
-a：显示隐藏文件
-i：显示 inode
-t：按时间排序
路径：/home
```

------

## 2. 用到的知识点

主要用：

```c
int main(int argc, char *argv[])
```

例如输入：

```bash
./myls -al /home
```

在程序里大概是：

```text
argc = 3

argv[0] = "./myls"
argv[1] = "-al"
argv[2] = "/home"
```

也就是说：

```text
argv[0] 是程序名
argv[1] 开始才是用户输入的参数
```

------

## 3. 推荐设计一个 Options 结构体

```c
typedef struct {
    int show_all;       // -a
    int long_format;    // -l
    int recursive;      // -R
    int sort_time;      // -t
    int reverse;        // -r
    int show_inode;     // -i
    int show_blocks;    // -s
} Options;
```

通俗理解：

> 用户输入了哪个参数，就把对应开关打开。

比如用户输入：

```bash
./myls -alR
```

那么：

```text
show_all = 1
long_format = 1
recursive = 1
```

后面输出时就根据这些开关决定怎么显示。

------

## 4. 为什么要这样做？

因为这样程序会很清晰。

比如输出文件时：

```text
如果 show_inode == 1，就打印 inode
如果 show_blocks == 1，就打印 blocks
如果 long_format == 1，就用长格式输出
如果 recursive == 1，就递归子目录
```

这样每个参数都变成了一个“开关”。

------

## 5. 这个板块重点总结

你要掌握：

```text
1. argc 表示参数个数
2. argv 保存参数内容
3. -a -l 和 -al 都要支持
4. 以 "-" 开头的一般是选项
5. 不以 "-" 开头的一般是路径
6. 没有路径时，默认路径是 "."
```

------

# 四、板块 2：路径识别与处理

## 1. 这个板块要解决什么问题？

用户输入的路径可能是：

```bash
./myls
./myls /home
./myls /etc
./myls main.c
./myls /home /etc
```

你要能处理：

```text
没有路径
一个路径
多个路径
路径是目录
路径是普通文件
路径不存在
路径没有权限访问
```

------

## 2. 没有路径怎么办？

如果用户只输入：

```bash
./myls
```

你应该默认列出当前目录：

```text
.
```

也就是相当于：

```bash
./myls .
```

------

## 3. 怎么判断路径是文件还是目录？

用：

```c
lstat()
```

它会把文件信息保存到：

```c
struct stat
```

里面。

然后用：

```c
S_ISDIR(st.st_mode)
```

判断是不是目录。

常见判断还有：

```c
S_ISREG(st.st_mode)   // 普通文件
S_ISDIR(st.st_mode)   // 目录
S_ISLNK(st.st_mode)   // 符号链接
```

------

## 4. 为什么用 `lstat()` 而不是 `stat()`？

这是重点。

```text
stat()  会跟随符号链接
lstat() 不会跟随符号链接
```

比如：

```text
link -> realfile
```

如果用 `stat()`，你看到的是 `realfile` 的信息。

如果用 `lstat()`，你看到的是 `link` 本身的信息。

实现 `ls -l` 时，我们希望看到：

```text
link -> realfile
```

所以更推荐用：

```c
lstat()
```

------

## 5. 这个板块重点总结

```text
1. "." 表示当前目录
2. 路径可能是文件，也可能是目录
3. lstat 可以获取路径对应文件的属性
4. S_ISDIR 可以判断是不是目录
5. 实现 ls 时，lstat 比 stat 更适合处理符号链接
```

------

# 五、板块 3：目录读取

## 1. 这个板块要解决什么问题？

当路径是目录时，比如：

```bash
./myls /home
```

你要读取 `/home` 目录里的所有文件名。

------

## 2. 用到的函数

核心函数：

```c
opendir()
readdir()
closedir()
```

它们和你前面学的文件 I/O 很像：

```text
open()     打开普通文件
read()     读取普通文件
close()    关闭普通文件

opendir()  打开目录
readdir()  读取目录项
closedir() 关闭目录
```

------

## 3. 基本流程

```text
opendir(path)
    ↓
循环 readdir()
    ↓
拿到每个目录项的名字 entry->d_name
    ↓
closedir()
```

`entry->d_name` 是文件名，例如：

```text
.
..
main.c
Makefile
.git
src
```

------

## 4. `-a` 在这里实现

Linux 中隐藏文件的规则很简单：

```text
文件名以 "." 开头，就是隐藏文件
```

比如：

```text
.bashrc
.profile
.git
```

所以没有 `-a` 时：

```text
如果 name[0] == '.'
就跳过
```

有 `-a` 时才显示它们。

------

## 5. 为什么不能读一个就打印一个？

因为后面有排序：

```bash
ls -t
ls -r
ls -tr
```

排序必须先拿到全部文件。

所以正确流程是：

```text
读取所有目录项
    ↓
保存到数组
    ↓
排序
    ↓
统一输出
```

而不是：

```text
读一个
打印一个
```

------

## 6. 这个板块重点总结

```text
1. opendir 打开目录
2. readdir 一次读取一个目录项
3. closedir 关闭目录
4. entry->d_name 只是文件名，不是完整路径
5. -a 的本质就是是否跳过 "." 开头的文件
6. 为了排序，必须先保存所有文件信息
```

------

# 六、板块 4：文件属性获取

这是整个项目最核心的板块。

## 1. 这个板块要解决什么问题？

`ls` 不只是显示文件名，它还会显示：

```text
inode 号
文件大小
文件权限
文件类型
硬链接数
用户
组
修改时间
占用块数
```

这些信息全部来自：

```c
struct stat
```

------

## 2. 推荐设计 FileInfo 结构体

```c
typedef struct {
    char *name;          // 文件名，例如 main.c
    char *path;          // 完整路径，例如 ./main.c
    struct stat st;      // 文件属性
} FileInfo;
```

通俗理解：

> 每读到一个文件，就给它做一张“信息卡”。

这张信息卡里保存：

```text
它叫什么名字
它的完整路径是什么
它的 inode 是多少
它的权限是什么
它的大小是多少
它的修改时间是什么
```

------

## 3. 为什么既要保存 name，又要保存 path？

因为它们用途不同。

```text
name：用来显示
path：用来 lstat、readlink、递归
```

比如你正在读取 `/home/user` 目录，读到一个文件名：

```text
a.txt
```

显示时可以显示：

```text
a.txt
```

但获取属性时要用完整路径：

```text
/home/user/a.txt
```

所以你需要一个路径拼接函数：

```text
join_path(dir, name)
```

------

## 4. `struct stat` 中的重要字段

| 字段        | 作用           | 用在哪个功能     |
| ----------- | -------------- | ---------------- |
| `st_ino`    | inode 号       | `-i`             |
| `st_blocks` | 占用块数       | `-s`             |
| `st_mode`   | 文件类型和权限 | `-l`、颜色、递归 |
| `st_nlink`  | 硬链接数       | `-l`             |
| `st_uid`    | 用户 ID        | `-l`             |
| `st_gid`    | 组 ID          | `-l`             |
| `st_size`   | 文件大小       | `-l`             |
| `st_mtime`  | 修改时间       | `-t`、`-l`       |

------

## 5. 这个板块重点总结

```text
1. struct stat 是实现 ls 的核心
2. st_ino 用于 -i
3. st_blocks 用于 -s
4. st_mode 用于判断类型和权限
5. st_mtime 用于 -t 排序
6. st_size 是文件大小
7. st_blocks 是磁盘占用块数
8. name 用来显示，path 用来操作
```

------

# 七、板块 5：普通格式输出

## 1. 这个板块要解决什么问题？

没有 `-l` 时，普通输出文件名。

例如：

```bash
./myls
```

输出：

```text
main.c  Makefile  src
```

如果加 `-i`：

```bash
./myls -i
```

输出：

```text
123456 main.c
123457 Makefile
```

如果加 `-s`：

```bash
./myls -s
```

输出：

```text
4 main.c
8 Makefile
```

如果组合：

```bash
./myls -is
```

输出：

```text
123456 4 main.c
123457 8 Makefile
```

------

## 2. `-i` 怎么实现？

打印：

```c
st.st_ino
```

通俗理解：

> inode 是文件在文件系统里的身份证号。

文件名只是给人看的名字。

同一个 inode 甚至可能有多个文件名，也就是硬链接。

------

## 3. `-s` 怎么实现？

打印：

```c
st.st_blocks
```

但是注意：

```text
st_blocks 通常以 512 字节为单位
ls -s 通常显示 1K 块数
```

所以可以显示：

```text
st_blocks / 2
```

或者更稳一点：

```text
(st_blocks + 1) / 2
```

------

## 4. 普通输出为什么也要考虑对齐？

如果不对齐，输出可能很乱：

```text
1 a.c
123456 very_long_name.c
22 b.c
```

可以先简单实现，后面在美化板块里优化。

------

## 5. 这个板块重点总结

```text
1. -i 显示 inode
2. -s 显示占用块数
3. st_size 是文件大小
4. st_blocks 是实际占用磁盘块数
5. 普通输出也要考虑颜色和对齐
```

------

# 八、板块 6：`-l` 长格式输出

这是最重要、最综合的一块。

## 1. `ls -l` 显示什么？

系统 `ls -l` 类似这样：

```text
-rw-r--r-- 1 user group 1234 2026-05-14 10:30 main.c
drwxr-xr-x 2 user group 4096 2026-05-14 10:31 src
lrwxrwxrwx 1 user group    5 2026-05-14 10:32 link -> a.txt
```

每一列含义：

```text
文件类型和权限
硬链接数
用户名
组名
文件大小
修改时间
文件名
```

------

## 2. 文件类型怎么判断？

使用：

```c
st.st_mode
```

常见判断：

```c
S_ISREG(st.st_mode)   // 普通文件
S_ISDIR(st.st_mode)   // 目录
S_ISLNK(st.st_mode)   // 符号链接
S_ISCHR(st.st_mode)   // 字符设备
S_ISBLK(st.st_mode)   // 块设备
S_ISFIFO(st.st_mode)  // 管道
S_ISSOCK(st.st_mode)  // socket
```

显示字符：

| 类型     | 字符 |
| -------- | ---- |
| 普通文件 | `-`  |
| 目录     | `d`  |
| 符号链接 | `l`  |
| 字符设备 | `c`  |
| 块设备   | `b`  |
| 管道     | `p`  |
| socket   | `s`  |

------

## 3. 权限怎么显示？

权限字符串一共 10 位：

```text
-rw-r--r--
drwxr-xr-x
lrwxrwxrwx
```

第 1 位是文件类型。

后 9 位是权限：

```text
用户权限    组权限    其他人权限
rwx         r-x       r-x
```

含义：

```text
r：可读
w：可写
x：可执行
-：没有对应权限
```

你需要写一个函数：

```c
mode_to_string()
```

把 `st_mode` 转成：

```text
-rw-r--r--
```

------

## 4. 用户名和组名怎么显示？

`st_uid` 和 `st_gid` 是数字。

比如：

```text
1000
1000
```

但 `ls -l` 一般显示：

```text
user group
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

如果找不到用户名或组名，就显示数字 ID。

------

## 5. 修改时间怎么显示？

`st_mtime` 是时间戳，不适合直接打印。

需要：

```c
localtime()
strftime()
```

可以格式化成：

```text
2026-05-14 10:30
```

不用一开始就完全模仿 GNU `ls` 的月份格式，先做到清晰可读。

------

## 6. 符号链接怎么显示？

如果文件是符号链接，`ls -l` 通常显示：

```text
link -> target
```

要用：

```c
readlink()
```

注意：

```text
readlink() 不会自动在字符串末尾加 '\0'
```

所以你必须手动补：

```c
buf[n] = '\0';
```

------

## 7. 这个板块重点总结

```text
1. -l 是整个 ls 中最综合的功能
2. st_mode 既保存文件类型，也保存权限
3. S_ISDIR、S_ISREG、S_ISLNK 用来判断类型
4. 权限字符串一共 10 位
5. getpwuid 把 uid 转用户名
6. getgrgid 把 gid 转组名
7. localtime 和 strftime 格式化时间
8. readlink 读取符号链接目标，但不会自动补 '\0'
```

------

# 九、板块 7：排序功能 `-t` 和 `-r`

## 1. 默认排序

正常 `ls` 默认按文件名排序。

可以用：

```c
strcmp()
```

比较文件名。

------

## 2. `-t` 怎么实现？

`-t` 表示按修改时间排序：

```text
新的文件排在前面
```

对应字段：

```c
st.st_mtime
```

要使用：

```c
qsort()
```

排序前，你必须已经把所有文件信息保存到数组里。

------

## 3. `-r` 怎么实现？

`-r` 表示反向排序。

比如默认：

```text
a b c
```

加 `-r`：

```text
c b a
```

如果 `-t` 的结果是：

```text
new old
```

加 `-tr` 后：

```text
old new
```

最简单的实现方式：

```text
先正常排序
如果没有 -r，从前往后输出
如果有 -r，从后往前输出
```

这样比把反向逻辑塞进比较函数里更好理解。

------

## 4. 为什么必须先保存再排序？

因为如果你：

```text
读一个
打印一个
```

那你根本不知道后面有没有更新的文件。

所以必须：

```text
全部读取
全部保存
统一排序
统一输出
```

------

## 5. 这个板块重点总结

```text
1. qsort 用于数组排序
2. 默认按文件名排序
3. -t 按 st_mtime 排序
4. -r 可以通过反向输出实现
5. 排序要求先保存所有文件信息
```

------

# 十、板块 8：递归功能 `-R`

## 1. `-R` 要做什么？

```bash
ls -R
```

表示递归显示目录。

例如：

```text
test:
a.txt src

test/src:
main.c utils.c
```

也就是：

```text
先显示当前目录
再进入子目录
继续显示
```

------

## 2. 递归逻辑

伪代码：

```text
list_dir(path):
    读取 path 目录
    获取文件信息
    排序
    输出当前目录内容

    如果有 -R:
        遍历刚才保存的文件
        如果它是目录:
            如果不是 "." 和 "..":
                list_dir(子目录路径)
```

------

## 3. 为什么必须跳过 `.` 和 `..`？

因为：

```text
.  表示当前目录
.. 表示上一级目录
```

如果你递归进入 `.`：

```text
当前目录 -> 当前目录 -> 当前目录 -> ...
```

会无限递归。

如果递归进入 `..`：

```text
当前目录 -> 上一级目录 -> 又可能回到当前目录
```

也会出问题。

所以必须跳过：

```text
.
..
```

------

## 4. 为什么不递归符号链接目录？

符号链接可能造成循环。

比如：

```text
a/link -> ../a
```

如果你跟着这个链接递归，可能永远绕圈。

所以建议：

```text
使用 lstat
只递归真正的目录
不递归符号链接目录
```

------

## 5. `/` 遍历测试要注意什么？

验收要求说：

```text
-R 需要对 / 的遍历测试
```

测试命令：

```bash
./myls -alR /
```

这个测试会遇到很多特殊情况：

```text
权限不足
/proc 中动态变化的文件
/sys 中的特殊文件
符号链接
设备文件
```

你的程序不能因为一个目录打不开就退出。

正确做法：

```text
打印错误
继续处理其他目录
```

比如：

```text
myls: cannot open '/root': Permission denied
```

然后继续遍历。

------

## 6. 这个板块重点总结

```text
1. -R 的核心是递归
2. 递归必须有停止条件
3. 必须跳过 "." 和 ".."
4. 使用 lstat 可以避免跟随符号链接
5. 遍历 / 时遇到错误不能崩溃
```

------

# 十一、板块 9：美化、对齐、颜色、资源释放

## 1. 输出对齐

`ls -l` 看起来整齐，是因为它会对齐列。

例如：

```text
-rw-r--r-- 1 user group     12 2026-05-14 10:30 a.c
-rw-r--r-- 1 user group 123456 2026-05-14 10:30 very_long_file.c
```

文件大小这一列是右对齐的。

你可以先统计最大宽度：

```text
最大 inode 宽度
最大 block 宽度
最大 link 数宽度
最大 size 宽度
最大用户名长度
最大组名长度
```

然后用：

```c
printf("%*ld", width, value);
```

`%*ld` 的意思是：

```text
宽度由参数指定，不够就补空格
```

------

## 2. 颜色显示

可以使用 ANSI 转义序列。

建议：

| 文件类型   | 颜色   |
| ---------- | ------ |
| 目录       | 蓝色   |
| 可执行文件 | 绿色   |
| 符号链接   | 青色   |
| 普通文件   | 默认色 |

常见颜色码：

```text
蓝色：\033[1;34m
绿色：\033[1;32m
青色：\033[1;36m
恢复：\033[0m
```

可以封装一个函数：

```c
print_name_color()
```

根据文件类型决定颜色。

------

## 3. 资源释放

验收要求里明确说：

```text
无资源与内存泄漏
```

所以你必须注意：

```text
opendir 之后要 closedir
malloc 之后要 free
realloc 之后最终也要 free
strdup 之后要 free
```

如果你有：

```c
FileInfo *files;
```

每个 `FileInfo` 里面有：

```c
char *name;
char *path;
```

释放时应该：

```text
先释放每个 files[i].name
再释放每个 files[i].path
最后释放 files 数组
```

------

## 4. 使用 valgrind 检查

编译：

```bash
gcc -g -Wall -Wextra myls.c -o myls
```

检查：

```bash
valgrind --leak-check=full ./myls -alR testdir
```

希望看到：

```text
definitely lost: 0 bytes
indirectly lost: 0 bytes
```

------

## 5. 这个板块重点总结

```text
1. printf 的宽度控制可以实现对齐
2. ANSI 转义序列可以实现颜色
3. 资源泄漏不只是内存泄漏，目录流也要关闭
4. malloc、strdup、opendir 都要有对应释放
5. valgrind 是检查内存问题的重要工具
```

------

# 十二、推荐代码结构

刚开始可以只写一个文件：

```text
myls.c
```

等功能完成后，再考虑拆成多个文件。

最终可以拆成：

```text
myls/
├── main.c        // main 函数、参数解析
├── myls.h        // 结构体和函数声明
├── list.c        // 目录读取、路径处理
├── print.c       // 输出相关
├── sort.c        // 排序相关
├── utils.c       // 工具函数
└── Makefile
```

初学阶段推荐先写一个 `myls.c`，因为拆太早容易分散注意力。

------

# 十三、建议的核心函数

你最终可以设计这些函数：

```c
parse_options()
```

作用：

```text
解析 -a -l -R -t -r -i -s
```

------

```c
list_path()
```

作用：

```text
处理一个路径，判断它是文件还是目录
```

------

```c
list_dir()
```

作用：

```text
列出一个目录，是整个程序的核心函数
```

------

```c
read_dir_entries()
```

作用：

```text
读取目录，把所有文件保存到 FileInfo 数组中
```

------

```c
sort_entries()
```

作用：

```text
根据 -t 排序
```

------

```c
print_entries()
```

作用：

```text
根据选项统一输出文件列表
```

------

```c
print_long()
```

作用：

```text
输出 -l 格式
```

------

```c
mode_to_string()
```

作用：

```text
把 st_mode 转成 -rw-r--r--
```

------

```c
format_time()
```

作用：

```text
把 st_mtime 转成人能看懂的时间
```

------

```c
join_path()
```

作用：

```text
把目录路径和文件名拼成完整路径
```

------

```c
free_entries()
```

作用：

```text
释放 FileInfo 数组中的内存
```

------

# 十四、推荐实现顺序

不要一开始直接写最终版。

建议分成 5 个版本。

------

## 版本 1：最小可运行版

目标：

```bash
./myls
./myls /home
./myls /home /etc
```

实现内容：

```text
1. 读取命令行路径
2. 没有路径时默认 "."
3. opendir 打开目录
4. readdir 读取文件名
5. closedir 关闭目录
6. 遇到错误不崩溃
```

这一版只解决：

> 我能不能把目录里的文件名读出来？

------

## 版本 2：加入参数解析和 `-a`

目标：

```bash
./myls -a
./myls -a /home
./myls -alR
```

实现内容：

```text
1. Options 结构体
2. parse_options 函数
3. 支持 -a -l 和 -al 两种写法
4. 先只让 -a 生效
```

这一版重点是：

> 我能不能正确识别用户输入的参数？

------

## 版本 3：加入 `lstat`、`-i`、`-s`

目标：

```bash
./myls -i
./myls -s
./myls -is
./myls -ais /home
```

实现内容：

```text
1. FileInfo 结构体
2. lstat 获取文件属性
3. -i 显示 st_ino
4. -s 显示 st_blocks
```

这一版重点是：

> 我能不能获取每个文件的属性？

------

## 版本 4：加入排序 `-t`、`-r`

目标：

```bash
./myls -t
./myls -r
./myls -tr
./myls -ai -t /home
```

实现内容：

```text
1. 把文件信息保存到数组
2. qsort 默认按名字排序
3. -t 按 st_mtime 排序
4. -r 反向输出
```

这一版重点是：

> 我能不能在输出前整理和排序数据？

------

## 版本 5：加入 `-l`、`-R`、颜色和对齐

目标：

```bash
./myls -l
./myls -al
./myls -alR /
./myls -i /home /etc
```

实现内容：

```text
1. mode_to_string 解析权限
2. getpwuid/getgrgid 显示用户和组
3. localtime/strftime 显示时间
4. readlink 显示符号链接
5. 递归实现 -R
6. 输出对齐
7. 颜色显示
8. valgrind 检查
```

这一版重点是：

> 我能不能做出一个比较完整、稳定、美观的 ls？

------

# 十五、最终验收测试命令

基础测试：

```bash
./myls
./myls -a
./myls -l
./myls -R
./myls -t
./myls -r
./myls -i
./myls -s
```

组合测试：

```bash
./myls -a -l
./myls -al
./myls -a /home
./myls -alR /
./myls -t
./myls -ai -t /home
./myls -i /home /etc
./myls -lsrt /etc
```

和系统命令对比：

```bash
ls -a -l
ls -a /home
ls -alR /
ls -t
ls -ai -t /home
ls -i /home /etc
```

内存检查：

```bash
valgrind --leak-check=full ./myls -alR testdir
```

------

# 十六、博客记录建议

你的博客可以按这个结构写：

```text
一、任务要求
二、整体设计思路
三、命令行参数解析
四、目录读取 opendir/readdir/closedir
五、文件属性获取 lstat 和 struct stat
六、实现 -a、-i、-s
七、实现 -t、-r 排序
八、实现 -l 长格式显示
九、实现 -R 递归遍历
十、颜色显示和输出对齐
十一、内存泄漏检查
十二、遇到的问题和解决方法
十三、总结
```

每一节都可以按照这个模板写：

```text
这个功能是什么？
为什么需要它？
用了哪些函数？
核心实现思路是什么？
遇到了什么问题？
如何测试？
```

------

# 十七、你现在第一步要做什么？

现在不要直接写 `-alR /`。

第一步只做：

```text
版本 1：最小可运行版
```

目标：

```bash
./myls
./myls /home
./myls /home /etc
```

只用这些知识：

```text
argc / argv
opendir
readdir
closedir
perror
```

等这个版本跑通，再加：

```text
-a
lstat
FileInfo
-i
-s
-t
-r
-l
-R
颜色
对齐
```

你要记住这个项目的核心一句话：

> `ls` 的本质是：读取目录项，获取文件属性，按照参数规则排序和格式化输出。