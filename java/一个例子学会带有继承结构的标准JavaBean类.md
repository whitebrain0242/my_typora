# 一个例子学会带有继承结构的标准JavaBean类

此文是自己学习黑马程序员的练习，为了巩固练习自己所学

## 题目如下



![image-20260327122349118](C:\Users\30473\AppData\Roaming\Typora\typora-user-images\image-20260327122349118.png)

## 第一步 画图

1,分类（同一类，有共性内容
         2,往上抽取共性

<img src="C:\Users\30473\AppData\Roaming\Typora\typora-user-images\image-20260327122932529.png" alt="image-20260327122932529" style="zoom: 67%;" />

##  第二步 写父类

注意要从上往下写，先写父类再写子类

```c
public class Person {
    //属性
    private int age;//把数据“藏起来”，通过方法来控制访问
    private String name;
    //方法
    public Person() {//alt+insert none
    }

    public Person(int age, String name) {
        this.age = age;
        this.name = name;//alt+insert+ctrl+a
    }
//alt+insert+setorget
    public int getAge() {
        return age;
    }

    public void setAge(int age) {
        this.age = age;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }
    //行为
    public void eat() {
        System.out.println("吃饭");
    }
    public void sleep() {
        System.out.println("睡觉");
    }
}
```

### private

**“自己类”** 指的是：

>  **定义这个 `private` 成员的那个类本身**，自己类 = 当前 class 里面



`private` 是一种**访问修饰符（access modifier）**，它的核心作用是：

👉 **控制访问权限，让成员只能在“自己类内部”使用**

#### 作用

**1️⃣ 限制访问范围（最严格）**：

被 `private` 修饰的变量、方法或构造函数：

-  ✅ 只能在**当前类内部**访问
-  ❌ 其他类（包括子类）都不能直接访问



**2️⃣ 实现封装（Encapsulation）**：

👉 把数据“藏起来”，通过方法来控制访问

✔ 好处：

-  防止数据被随意修改
-  可以加入校验逻辑（更安全）



**3️⃣ 隐藏实现细节**

private 方法只是“对外隐藏”，不是不执行

**4️⃣ 保护类的完整性**

private 防止“非法修改”，只能走安全逻辑

### this

```c
class Person {
    private int age;

    public void setAge(int age) {
        this.age = age;
    }
}
```

`this.age = age;`

this.age:成员变量（类里的）

age:参数（方法里的）

## 第三步 写子类

```c
public class Student extends Person {
    private String grade;

    public Student(){
    }
    public Student(String name,int age,String grade) {
        super(age, name);
        this.grade = grade;//alt+insert+ctrl+a+enter+ctrl+a
    }//把上面那个参数删了
        public String getGrade(){
            return grade;
        }
            public void setGrade(String grade){
                this.grade=grade;
           }
                //行为
                public void study(){
                    System.out.println("学生正在学习中");
                }
}
```

```c
public class Teacher extends Person {
    private String subject;

    public Teacher() {

    }

    public Teacher(int age, String name, String subject) {
        super(age, name);
        this.subject = subject;
    }

    public String getSubject() {
        return subject;
    }

    public void setSubject(String subject) {
        this.subject = subject;
    }
    public void study(){
        System.out.println( "老师在教书");
    }
}

```

 

```c
public class BachelorStudent extends Student{
//私有化成员变量不写没有独有的
//空参构造
//带全部参数的构造(同接父类+直接父类+自己)
public BachelorStudent(){no usages
}
public Bachelorstudent(String name, int age,String grade){ 
super(name,age,grade);
}
//get/set
//重写学习的方法
@override no usages//alt+insert+override
public void study(){
System.out.println("本科的同学正在攻读本科内容中);
                   }
                   }
```

