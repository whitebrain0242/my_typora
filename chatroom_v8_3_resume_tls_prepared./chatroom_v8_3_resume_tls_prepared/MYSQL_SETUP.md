# MySQL Setup — v8.3

## Schema

v8.3 没有新增 MySQL table。

如果已经是 v8.2：

```text
001 users
002 friends/events
003 messages
004 groups/offline delivery
005 file transfers
```

不需要新 migration。

全新安装仍可使用：

```bash
sudo mysql < MYSQL_RUN_THIS_V8_2.sql
```

文件名保留 v8.2，是因为 v8.3 没有 schema 变化。

## v8.3 真正改变的是 C++ access layer

旧：

```text
SQL string + escape + runtime concatenation
```

新：

```text
MYSQL_STMT
mysql_stmt_prepare
mysql_stmt_bind_param
mysql_stmt_execute
mysql_stmt_bind_result
mysql_stmt_fetch
```

请看：

```text
MYSQL_PREPARED_GUIDE.md
```

## 检查

```bash
./tests/check_mysql_prepared.sh \
  src/mysql_database.cpp
```

应该：

```text
MySQL prepared-statement source contract passed
```
