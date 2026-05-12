# MySQL

登录8.0

```c
>mysql -u root -p -P 3306
```

#### 练习 1：连接 MySQL 数据库

**作用**：打开命令行，登录你的 MySQL 服务

```
mysql -u root -p -P 3306
```

-  输入密码后回车，看到欢迎信息 = 连接成功

#### 练习 2：查看所有数据库

**作用**：查看 MySQL 里已有的所有数据库

```
show databases;
```

#### 练习 3：创建新数据库

**作用**：新建一个名为`dbtest1`的数据库

```
create database dbtest1；
```

#### 练习 4：使用指定数据库

**作用**：进入`dbtest1`数据库，后续操作都在这个库中

```
use dbtest1;
```

#### 练习 5：创建数据表

**作用**：创建`employees`员工表，包含 id、姓名两个字段

```
-- 创建表：id是数字类型，name是字符串类型
create table employees(id int ,name varchar(15));
```

**配套小命令**

查看当前库中的所有表

```
show tables;
```

查看表中数据（空表）

```
select * from employees;
```

#### 练习 6：向表中插入数据 + 查询数据

① 插入第一条数据

```
insert into employees value (1001,'Tom');
```

② 插入第二条数据

```
insert into employees values (1002,'shake');
```

③ 插入第三条数据（支持中文）

```
insert into employees values(10003,'小白');
```

④ 查询所有数据（最终结果）

```
select * from employees;
```