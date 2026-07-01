# C++



## 基础入门

### 基础语法

#### 作用域与命名空间

##### 作用域scope是什么？

其实就是一个变量有效的空间，举例如下：

```c++
int global = 1;           // 全局作用域

void func() {
    int local = 2;        // 局部作用域
    if (true) {
        int block = 3;    // 块作用域（if 语句内）
        global = 10;      // 可以访问全局
    }
    // block 在这里已经不可见
}

int main() {
    func();
    // local 不可见
    // block 不可见
    global = 20;          // 全局仍然可见
}
```

##### 命名空间是什么？

它的主要目的是**避免名字冲突**（尤其是当使用多个库时）。









#### using

`using` 是 C++ 中的一个**关键字**，它有多种用途。根据语境不同，它可以用来：

1.  **引入命名空间中的名字**（`using 声明` 和 `using 指令`）
2.  **创建类型别名**（替代 `typedef`，C++11 起）
3.  **在派生类中引入基类成员**（改变访问权限或解决重载问题）



##### 一、`using` 与命名空间

这是最常见的用法，用于避免反复写 `namespace::`。

###### 1. `using` 声明 (using declaration)

**语法：** `using 命名空间::名字;`

将指定的名字引入当前作用域，之后可以直接使用该名字。

```
#include <iostream>
#include <vector>

int main() {
    using std::cout;   // 引入 cout
    using std::endl;   // 引入 endl
    cout << "Hello" << endl;   // 不用写 std::cout

    // 也可以引入类型
    using std::vector;
    vector<int> v = {1,2,3};
}
```



**特点：**

-   引入的单个名字在当前作用域可见。
-   如果当前作用域已有同名实体，会发生**冲突**（编译错误）。
-   比 `using namespace` 更安全，因为只引入需要的名字。

###### 2. `using` 指令 (using directive)

**语法：** `using namespace 命名空间名;`

将整个命名空间的所有名字**引入当前作用域**（确切说是使它们看起来像在当前作用域声明的一样）。

cpp

```
#include <iostream>
#include <vector>

int main() {
    using namespace std;   // 引入整个 std 命名空间
    cout << "Hello" << endl;   // 可以
    vector<int> v;              // 可以
}
```



**特点：**

-   简单粗暴，但容易造成名字冲突。
-   **强烈不建议**在头文件中使用（会污染全局空间）。
-   在 `.cpp` 文件中谨慎使用（最好只在局部作用域或函数内部使用）。

------

##### 二、`using` 作为类型别名（C++11）

C++11 引入了 `using` 类型别名语法，可以完全替代 `typedef`，而且语法更清晰（尤其是函数指针、模板别名时优势明显）。

###### 语法：`using 新名字 = 已有类型;`

cpp

```
// 传统 typedef
typedef unsigned long long uint64;
// 现代 using
using uint64 = unsigned long long;

// 复杂类型：函数指针
typedef void (*FuncPtr)(int, double);
using FuncPtr = void (*)(int, double);   // 更易读

// 模板别名（typedef 做不到）
template<typename T>
using Vec = std::vector<T>;   // Vec<int> 等价于 std::vector<int>
```



**与 `typedef` 对比：**

| 能力         | `typedef`     | `using` 别名 |
| :----------- | :------------ | :----------- |
| 简单类型别名 | ✅             | ✅            |
| 函数指针     | ✅（但语法难） | ✅（更清晰）  |
| 模板别名     | ❌             | ✅            |
| 作用域规则   | 相同          | 相同         |

------

##### 三、`using` 在继承中引入基类成员

在派生类中，可以用 `using` 将基类的**构造函数**或**成员函数**引入到派生类作用域（通常用于改变访问权限或重新暴露被隐藏的重载）。

###### 1. 引入基类构造函数（C++11）

让派生类直接继承基类的所有构造函数（而不是手动定义）。

cpp

```
struct Base {
    Base(int x) {}
    Base(double d) {}
};

struct Derived : Base {
    using Base::Base;   // 继承 Base 的所有构造函数
    // 相当于自动生成 Derived(int) 和 Derived(double)
};

Derived d1(10);    // OK
Derived d2(3.14);  // OK
```



###### 2. 引入基类成员函数（解决名字隐藏）

如果派生类定义了同名函数（即使参数不同），会隐藏基类的所有同名重载。可以用 `using` 把基类的重载也引入进来。

cpp

```
struct Base {
    void f(int) {}
    void f(double) {}
};

struct Derived : Base {
    using Base::f;    // 引入 Base 的所有 f 重载
    void f(char) {}   // 增加一个新的重载
};

int main() {
    Derived d;
    d.f(10);    // 调用 Base::f(int)
    d.f(3.14);  // 调用 Base::f(double)
    d.f('a');   // 调用 Derived::f(char)
}
```



###### 3. 改变访问权限

基类中的 `protected` 成员可以通过 `using` 在派生类中变为 `public`（仅限该派生类）。

cpp

```
struct Base {
protected:
    void protectedFunc() {}
};

struct Derived : Base {
public:
    using Base::protectedFunc;   // 现在对外公开
};

int main() {
    Derived d;
    d.protectedFunc();   // OK，原来不可访问
}
```



------

##### 四、总结：`using` 的三种角色

| 用途                     | 示例                                               | 说明                        |
| :----------------------- | :------------------------------------------------- | :-------------------------- |
| **命名空间引入**（声明） | `using std::cout;`                                 | 引入单个名字                |
| **命名空间引入**（指令） | `using namespace std;`                             | 引入整个命名空间（谨慎）    |
| **类型别名**（C++11）    | `using int64 = long long;`                         | 比 `typedef` 更现代         |
| **模板别名**             | `template<typename T> using Vec = std::vector<T>;` | `typedef` 做不到            |
| **继承构造函数**         | `using Base::Base;`                                | C++11，继承所有基类构造函数 |
| **引入基类成员函数**     | `using Base::func;`                                | 解决名字隐藏问题            |
| **改变访问权限**         | `using Base::protectedMember;` 放在 `public:` 下   | 提升可访问性                |

------

##### 五、常见误区提醒

-   **`using namespace std;`** 在初学者代码中常见，但大型项目、头文件中应该避免。推荐使用 `std::` 前缀或 `using std::cout;` 等局部声明。
-   **`using` 类型别名**和 `typedef` 的效果完全相同（除了模板别名）。现代 C++ 推荐使用 `using`，因为语法一致且更可读。
-   **`using` 不能把基类的私有成员引入**（因为私有成员原本就不可访问）。
-   `using` 在继承中引入的构造函数会**按照基类的定义**构造，不会合成默认参数等额外行为。



















#### 数据类型

```c
bool
nullptr==NULL
long long 
auto//自动推导类型
```

#### const

c语言中const是制度变量还是可以改变的，但是**c++是真的常量，不能改，必须初始化**

可以做数组长度

```c
const int N=10;
int arr[N];
```

```c
constexpr int a=10;//c++新增，编译的时候就确定下来，比const更加强大
```

#### 引用

```c
int a=10;
int &b=a;//b是a的别名，公用一块内存
```

1.b不是新变量，是a的别名

2.ab公用一块内存地址，修改任何一个另一个都会改变

#### 函数

 (1) 函数参数可以给**默认值**

```
void func(int a, int b = 10);  // C 没有
```

 (2) 函数**重载**（C 没有！C++ 核心）

同名函数，参数不同即可

```
void f(int);
void f(double);
void f(int, int);
```

 (3) 三种传参方式（C++ 完整对比）

① 值传递（和 C 一样）

```
void f(int x);
```

② 指针传递（和 C 一样）

```
void f(int* x);
```

③ **引用传递（C++ 新增，最常用！）**

```
void f(int& x);  // 直接传别名，不拷贝，安全，比指针好用
```

首先效率很高，因为不是拷贝的，节省内存开销，并且函数能直接修改原变量，比指针更加安全更加好用

✅ **C++ 优先用引用，不用指针！**

#### 数组

1.可以用const定义数组长度

2.代替数组`std::vector`   [跳转到vecor](#vector)

#### 字符串

```c
string s = "hello";
s1 + s2        // 拼接
s.size()       // 长度
s == s1        // 比较
s[0]           // 访问
```

#### 输入输出

```c
int a;
cin >> a;//从键盘读取用户输入的整数，存入变量 a
cout << a << endl;//把变量 a 的值输出到屏幕，然后换行

int x, y;
cin >> x >> y;         // 连续读入两个整数，用空格/回车分隔
cout << x << " " << y << endl; // 连续输出
```

-  `cin >>`：输入运算符，**箭头指向变量**，表示数据 “流” 向变量
-  `cout <<`：输出运算符，**箭头指向屏幕**，表示数据 “流” 向屏幕
-  `endl`：表示 “换行 + 刷新缓冲区”，等价于 `'\n'` 但会立刻清空输出缓存
-  变量 `a` 定义时没有初始化，但 `cin >> a` 会给它赋值，所以不会出问题





## 面向对象











## 核心特性和进阶语法

### Lamada表达式

参数是调用的时候就直接提供给你，但是捕获是现在存好，以后自己用，就不用传参数了，比如说线程就不需要传参数

```
auto dfs = [&](this auto&& dfs, /* 入参 */) -> /* 返回类型 */ {
    
};

this auto&& dfs：显式对象参数。

this 关键字标记了这是一个显式对象参数，表示它代表 lambda 对象本身。

auto&& 是参数的类型占位符，自动推导 lambda 对象的完整类型（包括 const、引用等修饰）。auto&& 是万能引用，可以保持 lambda 的值类别（左值或右值）。

dfs 是这个参数的名称，在 lambda 体内可以用这个名字来递归调用自身。
```



```c
[捕获] (参数) -> 返回值 { 函数体 };
```



**为什么要用入参而不是直接在lamada外面定义变量，而且&可以直接引用？**

因为捕获的话在整个lamada生命周期只有一份，不能保存每一层递归的值，但是参数可以

捕获适合在递归过程中不变或者需要共享的数据：比如说sum等

参数适合在递归每一层值都不一样的数据，如当前节点，当前深度







捕获列表[]

```c
int a = 10;

[]() { /* 拿不到外面的 a */ };
[=]() { /* 拿到外面所有变量（拷贝） */ };
[&]() { /* 拿到外面所有变量（引用，可修改） */ };
[a]() { /* 只拿 a */ };
[&a]() { /* 只拿 a 的引用，可修改 */ };
```

参数（）

```c
[](int x, int y) {
    return x + y;
};
```

返回值（可省略

```c
[](int x, int y) -> int {
    return x + y;
};
```

{代码}里面是函数内容

## 库和工具类

### STL

（Standard Template Library，标准模板库）

#### **Container容器类**

| 容器             | 特点             |
| ---------------- | ---------------- |
| `vector`         | 动态数组，最常用 |
| `array`          | 固定长度数组     |
| `list`           | 双向链表         |
| `deque`          | 双端队列         |
| `stack`          | 栈（后进先出）   |
| `queue`          | 队列（先进先出） |
| `priority_queue` | 优先队列（堆）   |
| `set`            | 自动排序且不重复 |
| `multiset`       | 自动排序可重复   |
| `map`            | 键值对，自动排序 |
| `unordered_set`  | 哈希集合         |
| `unordered_map`  | 哈希表           |

##### vector

<a id="vector"></a>

**动态数组**

3个特点

**动态扩容**：不够大了自动变大

**知道自己的长度**：`v.size()`

**和数组用法几乎一样**：`v[i]`

```c
1. 头文件
#include <vector>   // 必须加
using namespace std;
//如果是这样的
vector<int>& nums
   //那么其实表示就是直接使用原数组而不是拷贝一个数组，这样效率更快，也可正常进行调用

2. 创建 vector
vector<int> v;        // 空的
vector<int> v(10);    // 大小 10，默认都是 0
vector<int> v = {1,2,3}; // 直接赋值
vector<int> v2 = v;//拷贝另外一个vector

3. 添加元素
     //尾部
//先创建对象再拷贝进去
v.push_back(10);
v.push_back(20);
//直接在vector内部创建，不拷贝，效率更高
v.emplace_back(30);
v.insert(v.begin() + 1, 99); // 第 1 位插入 99，插入元素

4. 访问元素（和数组一样）
cout << v[0];    // 第 0 个元素
cout << v.at(1); // 安全访问，越界会报错
v.front();//取第一个元素
v.back();//取最后一个元素

5. 查看大小
cout << v.size(); // 元素个数
v.capacity();//总容量

6. 遍历（超级常用）
for(int i=0; i<v.size(); i++){
    cout << v[i] << endl;
}

7 清空
v.clear();

8. 判断是否为空
if(v.empty()){ ... }

9.调整大小
 v.resize(20);

10.预留空间
v.reserve(100);

11.删除元素
v.pop_back();//删除最后一个
v.erase(v.begin() + 2); // 删除第 2 个，删除指定位置

12.算法
 sort(v.begin(), v.end());//排序
reverse(v.begin(), v.end());//反转
auto it = find(v.begin(), v.end(), 10);//查找
```



#### 算法Algorithms

```
头文件：

#include <algorithm>

STL提供大量现成算法。

排序
std::sort(v.begin(), v.end());

例子：

std::vector<int> v = {5,1,3,2,4};

std::sort(v.begin(), v.end());

结果：

1 2 3 4 5
查找
auto it = std::find(
    v.begin(),
    v.end(),
    3
);
最大值
int mx = *std::max_element(
    v.begin(),
    v.end()
);
反转
std::reverse(
    v.begin(),
    v.end()
);
```



#### 迭代器

#### 函数对象Functors













### std::filesystem

##### 常用操作分类

###### 1. 路径信息获取（分解与组合）

| 操作                  | 示例代码                | 结果（假设 `p = "/home/user/file.txt"`） |
| :-------------------- | :---------------------- | :--------------------------------------- |
| 文件名（含扩展名）    | `p.filename()`          | `"file.txt"`                             |
| 扩展名                | `p.extension()`         | `".txt"`                                 |
| 不含扩展名的文件名    | `p.stem()`              | `"file"`                                 |
| 父路径                | `p.parent_path()`       | `"/home/user"`                           |
| 根目录（Unix 为 `/`） | `p.root_directory()`    | `"/"`                                    |
| 拼接路径              | `p / "sub" / "new.txt"` | `"/home/user/file.txt/sub/new.txt"`      |
| 转换为字符串          | `p.string()`            | `"/home/user/file.txt"`                  |

**示例**：

cpp

```
fs::path p = "/home/user/docs/notes.txt";
std::cout << p.stem() << '\n';      // notes
std::cout << p.extension() << '\n'; // .txt
fs::path parent = p.parent_path();  // /home/user/docs
fs::path full = parent / "archive" / "old.txt";
std::cout << full; // /home/user/docs/archive/old.txt
```



------

###### 2. 文件/目录状态查询

| 操作             | 示例代码                 | 说明                       |
| :--------------- | :----------------------- | :------------------------- |
| 是否存在         | `fs::exists(p)`          | 返回 `bool`                |
| 是否是普通文件   | `fs::is_regular_file(p)` |                            |
| 是否是目录       | `fs::is_directory(p)`    |                            |
| 是否是符号链接   | `fs::is_symlink(p)`      |                            |
| 文件大小         | `fs::file_size(p)`       | 返回 `uintmax_t`，单位字节 |
| 最后修改时间     | `fs::last_write_time(p)` | 返回 `file_time_type`      |
| 获取状态（批量） | `fs::status(p)`          | 返回 `file_status` 对象    |

cpp

```
fs::path p = "data.txt";
if (fs::exists(p)) {
    std::cout << "Size: " << fs::file_size(p) << " bytes\n";
    auto ftime = fs::last_write_time(p);
    // 转换为 time_t 打印较复杂，可借助 std::chrono
} else {
    std::cout << "File not found\n";
}
```



------

###### 3. 目录操作（创建、遍历、删除）

创建目录

cpp

```
fs::create_directory("mydir");            // 创建单层目录
fs::create_directories("a/b/c");          // 递归创建多层目录（自动创建父目录）
```

遍历目录

使用 **目录迭代器**：`directory_iterator` 和 `recursive_directory_iterator`。

cpp

```
// 遍历当前目录下的所有文件/目录（不递归）
for (const auto& entry : fs::directory_iterator(".")) {
    std::cout << entry.path() << '\n';
}

// 递归遍历所有子目录
for (const auto& entry : fs::recursive_directory_iterator(".")) {
    std::cout << entry.path() << '\n';
}
```



每个 `entry` 是 `directory_entry` 对象，可以调用 `path()`、`is_regular_file()` 等。

删除

cpp

```
fs::remove("file.txt");                 // 删除单个文件或空目录
fs::remove_all("some_directory");       // 递归删除目录及其所有内容（慎用）
```



------

###### 4. 文件操作（复制、移动、重命名）

cpp

```
fs::copy("from.txt", "to.txt");                     // 复制文件
fs::copy("from_dir", "to_dir", fs::copy_options::recursive); // 递归复制目录

fs::rename("oldname.txt", "newname.txt");           // 重命名或移动（跨目录也可）
```



------

###### 5. 当前路径与绝对路径

cpp

```
fs::path cur = fs::current_path();      // 获取当前工作目录
fs::current_path("/new/path");          // 设置当前工作目录

fs::path abs = fs::absolute("relative/path");       // 转为绝对路径
fs::path canon = fs::canonical("./../file.txt");    // 解析符号链接，返回规范化的绝对路径
```



##### 错误处理

大多数 `filesystem` 函数有**两个重载**：

-   抛出异常版本（默认）– 失败时抛出 `std::filesystem::filesystem_error`
-   接受 `std::error_code&` 的版本 – 不抛异常，通过 `error_code` 查看错误。

cpp

```
std::error_code ec;
fs::remove("somefile.txt", ec);
if (ec) {
    std::cerr << "删除失败: " << ec.message() << '\n';
}
```



推荐在关键路径上使用 `error_code` 版本，避免异常导致程序崩溃。

### std::signal



| 宏名                  | 含义                   | 默认行为              |
| :-------------------- | :--------------------- | :-------------------- |
| `SIGINT`              | 终端中断（Ctrl+C）     | 终止程序              |
| `SIGTERM`             | 终止请求（kill 默认）  | 终止程序              |
| `SIGSEGV`             | 段错误（非法内存访问） | 产生 core dump 并终止 |
| `SIGABRT`             | 调用 `std::abort()`    | 终止程序              |
| `SIGFPE`              | 浮点异常（除零等）     | 终止程序              |
| `SIGUSR1` / `SIGUSR2` | 用户自定义信号         | 终止程序              |

用法：

1. 注册自定义处理函数



```
std::signal(SIGINT, my_handler);   // 发生 SIGINT 时调用 my_handler
```

2. 恢复默认行为



```
std::signal(SIGINT, SIG_DFL);       // 变成默认行为（通常是终止程序）
```

3. 忽略信号



```
std::signal(SIGINT, SIG_IGN);       // 忽略 SIGINT，Ctrl+C 无效
```









































### **工具函数**

### **字符串**



## things

##### explicit

修饰构造函数，**禁止隐式类型转换**

专门加在**单参数构造函数**前面，防止代码不小心出错

```c
你写了一个 “人” 类：
class Person {
public:
    Person(int age) {
        // 用年龄创建一个人
    }
};
现在你写：
Person p = 18;

你觉得这行代码对吗？
18 是数字，怎么能直接变成一个人？
     
但编译器会偷偷帮你变：
18 → 自动调用 Person(18) → 变成一个人
这叫 隐式转换，非常离谱，也非常容易写错代码！
```

加上之后

```c
现在再写：
Person p = 18;  // ❌ 报错！不允许偷偷变！
必须明着写：
Person p(18);   // ✅ 明确创建一个人
```

##### std::size_t

标准库无符号整型，专门用于表示**容器大小、数组下标、数量**

##### using namespace std;

所有 std:: 都来自这里

##### 条件变量

1.等待：满足条件才醒

```
cv.wait(lock, [&]() {
            // 两个醒的条件：
            // 1. 有任务  OR  2. 要停止了
            return !taskQueue.empty() || stop;
        });
```

2.叫醒

叫醒一个正在等待的线程

```
 cv.notify_one();
 notify_all()
```

##### ♾️

```
int sum = INT_MIN;
```

##### 转大小写

转为大写

```
char ch = 'x';
char up = std::toupper(ch);
```

转为小写

```
char ch = 'X';
    char low = std::tolower(ch);
```

































# C++ 学习大纲（和你的 Java 结构对应）

## 一、基础入门（对应你 Java 的「初始 java」）

1. 开发环境搭建

   -  编译器（GCC/Clang/MSVC）、IDE 配置（VSCode/CLion/Visual Studio）
   -  编译、链接、运行的基本流程
   -  程序入口 `main()` 与头文件

   

2. 基础语法

   -  数据类型（`int`/`double`/`char`/`bool`/`long long`等）
   -  变量、常量与 `const` 关键字
   -  运算符、表达式、分支（`if-else`/`switch`）、循环（`for`/`while`/`do-while`）
   -  数组、字符串（C 风格字符串 `char*`）
   -  函数定义、声明、参数传递（值传递 / 指针传递 / 引用传递）、返回值

   

3. 指针与引用（C++ 核心基础）

   -  指针基础、地址与解引用
   -  空指针、野指针、指针数组、数组指针
   -  引用的概念、左值引用与右值引用

   

4. 内存管理基础

   -  栈内存 vs 堆内存
   -  `new` / `delete` 手动分配释放内存
   -  内存泄漏的概念与避免方式

   

------

## 二、面向对象（对应你 Java 的「高级」部分）

1. 类与对象

   -  类的定义、成员变量 / 成员函数
   -  访问权限控制（`public`/`private`/`protected`，对应 Java 权限修饰符）
   -  封装的实现（getter/setter、数据隐藏）
   -  构造函数、析构函数、拷贝构造函数、移动构造函数
   -  初始化列表、委托构造

   

2. 继承

   -  继承的概念、基类与派生类
   -  访问控制（`public`/`protected`/`private`继承）
   -  多继承与菱形继承问题（虚继承）

   

3. 多态

   -  静态多态（函数重载、运算符重载）
   -  动态多态（虚函数、`virtual`关键字）
   -  纯虚函数、抽象类（对应 Java 抽象类）
   -  虚析构函数、override/final 关键字

   

4. 接口与抽象类

   -  C++ 中没有`interface`关键字，用纯虚类实现接口
   -  抽象基类的设计模式

   

5. 内部类 / 嵌套类

   -  类内嵌套类的定义与访问规则
   -  局部类、匿名类的使用场景

   

------

## 三、C++ 核心特性与进阶语法

1. 模板（Template）

   -  函数模板、类模板
   -  模板特化、偏特化
   -  可变参数模板（C++11+）

   

2. STL 标准模板库（对应 Java 的集合框架）

   -  容器：`vector`/`list`/`deque`/`map`/`set`/`unordered_map`/`unordered_set`
   -  容器适配器：`stack`/`queue`/`priority_queue`
   -  算法库 `<algorithm>`：排序、查找、遍历、变换等
   -  迭代器（Iterator）的概念与使用

   

3. 智能指针（C++11+）

   -  `std::unique_ptr`/`std::shared_ptr`/`std::weak_ptr`
   -  RAII 资源管理思想

   

4. 异常处理

   -  `try`/`catch`/`throw` 语法
   -  标准异常类（`std::exception`及其子类）
   -  异常安全与 RAII

   

5. Lambda 表达式（对应 Java 的 Lambda）

   -  Lambda 语法、捕获列表
   -  `std::function` 与函数对象
   -  标准库中的函数对象（`std::bind`）

   

6. C++11/14/17/20 新特性

   -  `auto`/`decltype`/`nullptr`
   -  范围 for 循环、`constexpr`
   -  右值引用与移动语义、完美转发
   -  结构化绑定（C++17）、`std::optional`/`std::variant`（C++17）
   -  协程（C++20）

   

------

## 四、常用库与工具类（对应你 Java 的「API」部分）

1. 字符串处理

   -  `std::string` 常用操作（拼接、查找、替换、截取）
   -  字符串与数值类型转换
   -  C++17 `std::string_view`

   

2. 输入输出流（I/O）

   -  标准输入输出 `std::cin`/`std::cout`
   -  文件流 `std::ifstream`/`std::ofstream`/`std::fstream`
   -  格式化输出（`printf`/`std::format` C++20）

   

3. 日期与时间处理

   -  C++ 时间库 `<chrono>`
   -  时钟、时间点、时间段的使用

   

4. 数学工具类

   -  `<cmath>` 库：基础数学函数、常量
   -  随机数生成（`std::random` 替代 C 风格`rand()`）

   

5. 并发与多线程（C++11+）

   -  `std::thread` 线程创建与管理
   -  互斥锁 `std::mutex`、条件变量 `std::condition_variable`
   -  `std::future`/`std::promise`/`std::async`
   -  线程池的实现（就是你之前写的代码！）

   

6. 其他常用库

   -  内存操作 `<memory>`
   -  类型信息 `<typeinfo>`、类型特征 `<type_traits>`
   -  正则表达式 `<regex>`（C++11+）

   

------

## 五、进阶专题与实战（对应你 Java 的「小知识点」）

1. 设计模式在 C++ 中的实现

   -  单例模式、工厂模式、观察者模式等
   -  RAII 与资源管理模式

   

2. 性能优化

   -  编译优化选项（`-O2`/`-O3`）
   -  内存对齐、缓存友好代码
   -  拷贝消除、移动语义优化

   

3. 调试与错误处理

   -  GDB/LLDB 调试基础
   -  Valgrind 内存泄漏检测
   -  静态分析工具（Clang-Tidy）

   

4. 项目实战

   -  命令行工具开发
   -  简单的并发服务 / 客户端
   -  基于 STL 的数据结构实现
   -  跨平台编译（Makefile/CMake 基础）