# typedef的使用

typedef就是起别名，起外号的作用

## 用法

==typedef<数据类型>（别名）==

```c
// 为 int 类型创建别名 Integer
typedef int Integer;
```

```c
// 为 char* 创建别名 String
typedef char* String;
```

为结构体起别名

**方式1：先定义结构体，再起别名**

```c
struct Person {
     char name[20];
     int age;
};
typedef struct Person Person; // 为 struct Person 起别名 Person
```

**// 方式2：定义结构体的同时起别名（更常用）**

```c
typedef struct Student {
    char id[10];
    float score;
} Student;

```

**还可以省略结构体标签，直接起别名**

```c
typedef struct {
    char name[20];
    int age;
} Person;
```

**为函数指针起别名**

```c
typedef int (*CalcFunc)(int, int);
```

`CalcFunc` 是 “指向 返回值为 int、参数为两个 int 的函数” 的指针类型别名；

没有 `typedef` 的话，函数指针变量需要写成 `int (*func)(int, int)`，冗长且易出错。