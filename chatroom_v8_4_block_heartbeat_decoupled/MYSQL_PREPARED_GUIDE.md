# MySQL Prepared Statement Guide — v8.4

## 结论

`src/mysql_database.cpp` 已从：

```text
SQL string concatenation + escape
```

改成：

```text
MYSQL_STMT + ? placeholders + MYSQL_BIND
```

## 统一执行层

```text
Statement
SqlParam
bind_params
execute_prepared
query_prepared
```

所有业务方法都使用它们。

## 示例

用户是否存在：

```cpp
static constexpr const char* sql =
    "SELECT 1 "
    "FROM users "
    "WHERE username=? "
    "LIMIT 1";

query_prepared(
    connection_,
    sql,
    {
        SqlParam::text(username)
    },
    rows,
    error
);
```

文件 Protobuf BLOB：

```cpp
static constexpr const char* sql =
    "INSERT INTO file_transfers("
    "transfer_token,...,metadata"
    ") VALUES(?,?,?,?,?,?,?,?,?,?,?)";

execute_prepared(
    connection_,
    sql,
    {
        SqlParam::text(...),
        ...
        SqlParam::blob(
            std::move(bytes)
        )
    },
    &transfer_id,
    nullptr,
    error
);
```

## 静态契约

```bash
./tests/check_mysql_prepared.sh \
  src/mysql_database.cpp
```

当前应输出：

```text
MySQL prepared-statement source contract passed
```

源码审计当前：

```text
mysql_real_escape_string  0
escape(                   0
mysql_query(              0
mysql_real_query(         0
CLIENT_MULTI_STATEMENTS   0
std::to_string(           0
```

## 事务

```text
mysql_autocommit(0)
mysql_stmt_execute(...)
mysql_stmt_execute(...)
mysql_commit()
mysql_autocommit(1)
```

失败：

```text
mysql_rollback()
mysql_autocommit(1)
```

## 当前性能说明

虽然 SQL 已标准化为 Prepared Statement，但 `MySqlDatabase` 仍然是：

```text
one shared MYSQL connection
+
mutex
+
synchronous calls
```

因此正确性改善不等于 DB 已异步化。

后续性能版可以继续做：

```text
MySQL connection pool
+
business/DB worker pool
```

而不影响本版 Prepared Statement 接口思想。
