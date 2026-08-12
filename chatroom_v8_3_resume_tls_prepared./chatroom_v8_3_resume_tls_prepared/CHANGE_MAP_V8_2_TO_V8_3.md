# v8.2 → v8.3 精确修改清单

## 代码/配置/测试层

以 `chatroom_v8_2_file_transfer_official_protobuf.zip` 为基线，仅统计：

```text
CMakeLists.txt
include/
src/
proto/
sql/
config/*.example
scripts/
tests/
```

结果：

```text
修改 23 个
新增 11 个
删除 0 个
```

### 修改文件

```text
CMakeLists.txt
include/chat_server.hpp
include/config.hpp
include/file_transfer_service.hpp
include/integration/sqlite_client.hpp
include/minimuduo/net/TcpConnection.hpp
include/minimuduo/net/TcpServer.hpp
include/mysql_database.hpp
include/proto_types.hpp
proto/file_transfer.proto
sql/client_sqlite_schema.sql
src/chat_server.cpp
src/client.cpp
src/config.cpp
src/file_transfer_service.cpp
src/minimuduo/net/TcpConnection.cpp
src/minimuduo/net/TcpServer.cpp
src/mysql_database.cpp
src/server_main.cpp
src/sqlite_client.cpp
tests/test_file_transfer_storage.cpp
tests/test_proto.cpp
tests/test_sqlite_cache.cpp
```

### 新增文件

```text
config/tls_client.conf.example
config/tls_server.conf.example
include/minimuduo/net/TlsContext.hpp
include/tls_config.hpp
scripts/generate_dev_tls_cert.sh
src/minimuduo/net/TlsContext.cpp
tests/check_mysql_prepared.sh
tests/run_tls_context_test.sh
tests/run_tls_reactor_test.sh
tests/test_tls_context.cpp
tests/test_tls_reactor.cpp
```

### 删除文件

```text
(none)
```

完整逐行源码 diff：

```text
SOURCE_CHANGES_V8_2_TO_V8_3.patch
```

---

# 按模块看修改

## 1. TLS

新增：

```text
include/tls_config.hpp
include/minimuduo/net/TlsContext.hpp
src/minimuduo/net/TlsContext.cpp
config/tls_server.conf.example
config/tls_client.conf.example
scripts/generate_dev_tls_cert.sh
tests/test_tls_context.cpp
tests/test_tls_reactor.cpp
tests/run_tls_context_test.sh
tests/run_tls_reactor_test.sh
```

修改：

```text
include/minimuduo/net/TcpConnection.hpp
src/minimuduo/net/TcpConnection.cpp
include/minimuduo/net/TcpServer.hpp
src/minimuduo/net/TcpServer.cpp
include/config.hpp
src/config.cpp
src/server_main.cpp
src/client.cpp
CMakeLists.txt
```

关键变化：

```text
TCP accepted
→ SubReactor
→ nonblocking SSL_accept
→ ChatServer connection callback

readFd/writeFd
→ SSL_read/SSL_write (TLS path)
```

---

## 2. 断点续传

修改：

```text
proto/file_transfer.proto
include/proto_types.hpp
include/file_transfer_service.hpp
src/file_transfer_service.cpp
include/chat_server.hpp
src/chat_server.cpp
include/integration/sqlite_client.hpp
src/sqlite_client.cpp
src/client.cpp
sql/client_sqlite_schema.sql
tests/test_file_transfer_storage.cpp
tests/test_sqlite_cache.cpp
tests/test_proto.cpp
```

新增协议：

```text
FILE_READY <token> <offset>
FILE_PAUSED
FILE_RESUME_REQUEST
FILE_RESUME_START
```

新增持久状态：

```text
server:
tmp/<token>.part
tmp/<token>.resume.pb

client SQLite:
pending_uploads
partial_downloads
```

---

## 3. MySQL Prepared Statements

主要修改：

```text
include/mysql_database.hpp
src/mysql_database.cpp
```

新增测试：

```text
tests/check_mysql_prepared.sh
```

旧方式：

```text
runtime value
→ escape()
→ SQL string concat
→ mysql_real_query/mysql_query
```

新方式：

```text
static SQL with ?
→ MYSQL_STMT
→ MYSQL_BIND
→ mysql_stmt_execute
```

数据库实现中当前审计：

```text
mysql_real_escape_string = 0
escape(                  = 0
mysql_query(             = 0
mysql_real_query(        = 0
CLIENT_MULTI_STATEMENTS  = 0
std::to_string(          = 0
```

---

# 学习顺序

推荐按：

```text
V8_3_ADDED_CODE_GUIDE.md
↓
TLS_GUIDE.md
↓
RESUME_TRANSFER_GUIDE.md
↓
MYSQL_PREPARED_GUIDE.md
↓
SOURCE_CHANGES_V8_2_TO_V8_3.patch
```

这样先理解设计，再看逐行代码。
