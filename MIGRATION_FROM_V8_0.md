# v8.0 → v8.1 修改清单

本文件专门回答“改了哪些文件、哪些部分”。

## 1. CMakeLists.txt — 修改

新增依赖：

```cmake
pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)
```

`chat_core` 新增：

```text
src/redis_client.cpp
```

`chat_server` 链接 hiredis。

`chat_client` 新增：

```text
src/sqlite_client.cpp
```

并链接 sqlite3。

新增测试：

```text
test_sqlite_cache
test_redis_client
```

---

## 2. include/config.hpp — 修改

新增：

```cpp
struct RedisConfig
```

字段：

```text
host
port
password
database
connect_timeout_ms
presence_ttl_seconds
key_prefix
server_name
```

新增：

```cpp
load_redis_config()
```

---

## 3. src/config.cpp — 修改

把原来只服务 MySQL 的配置读取逻辑抽出：

```cpp
load_key_values()
```

MySQL 和 Redis 共用基础 key=value 解析。

新增 Redis 参数范围检查。

---

## 4. config/redis.conf.example — 新增

Redis 示例配置。

---

## 5. include/integration/redis_client.hpp — 大改

v8.0：

```text
只是虚接口占位
```

v8.1：

```text
真正 RedisClient
hiredis redisContext
mutex
presence API
unread API
自动重连辅助
```

---

## 6. src/redis_client.cpp — 新增

实现：

```text
连接
AUTH
SELECT
PING

SET NX EX presence
GET presence
Lua owner-safe EXPIRE
Lua owner-safe DEL

HINCRBY unread
HMGET unread
EXPIRE unread
```

---

## 7. include/chat_server.hpp — 修改

构造函数增加：

```cpp
RedisClient&
server_instance_id
presence_ttl_seconds
```

新增成员：

```text
redis_
server_instance_id_
presence_ttl_seconds_
presence_refresh_thread_
condition_variable
stopping_
```

新增辅助函数：

```text
presence_refresh_loop
refresh_all_presence_best_effort
claim_redis_presence
remove_redis_presence_best_effort
adjust_redis_unread_best_effort
send_redis_unread_summary_best_effort
```

---

## 8. src/chat_server.cpp — 修改

### constructor / destructor

启动、停止 Redis presence 刷新线程。

### on_connection(disconnected)

原来：

```text
只删除本机 online_users_
```

现在：

```text
删除本机 online_users_
+
owner-safe 删除 Redis presence
```

### handle_login

原来：

```text
密码正确
→ register_online_user
→ login success
```

现在：

```text
密码正确
→ register_online_user
→ Redis claim_presence
→ login success
```

登录后增加 Redis unread 摘要。

### handle_logout

增加 Redis presence 清理。

### handle_private_message

MySQL 创建 delivery 后：

```text
Redis private unread +1
```

在线实时发送完成、MySQL 标记 delivered 成功后：

```text
Redis private unread -1
```

### handle_group_message

每个群成员 delivery：

```text
Redis group unread +1
```

在线发送完成并标记 delivered：

```text
Redis group unread -1
```

### deliver_pending_messages

离线私聊/群聊重新投递完成后：

```text
MySQL mark delivered
→ Redis unread -1
```

### PENDING

增加 Redis unread cache 摘要。

---

## 9. src/server_main.cpp — 修改

增加：

```text
config/redis.conf
RedisClient
Redis connect + ping
```

新推荐启动格式：

```bash
./build/chat_server 9000 config/mysql.conf config/redis.conf 4
```

仍兼容：

```bash
./build/chat_server 9000 config/mysql.conf 8
```

---

## 10. include/integration/sqlite_client.hpp — 大改

v8.0：

```text
接口占位
```

v8.1：

```text
真实 SQLite 封装
LocalPrivateMessage
LocalGroupMessage
LocalCacheStats
```

---

## 11. src/sqlite_client.cpp — 新增

实现：

```text
sqlite3_open_v2
WAL
自动建表
prepared statements
私聊缓存
群聊缓存
私聊本地历史
群聊本地历史
统计
```

---

## 12. src/client.cpp — 大改

原来：

```text
recv()
→ 直接 cout.write()
```

现在：

```text
recv()
→ server_buffer
→ 按换行拆包
→ print
→ 更新当前登录账号
→ 识别私聊/群聊
→ SQLite cache
```

新增本地命令：

```text
LOCAL_HELP
LOCAL_DB
LOCAL_STATS
LOCAL_HISTORY_PRIVATE
LOCAL_HISTORY_GROUP
```

客户端第三个参数新增 SQLite 路径：

```bash
./build/chat_client 127.0.0.1 9000 data/alice.db
```

---

## 13. sql/client_sqlite_schema.sql — 新增

只用于阅读和调试。

客户端会自动建表，不要求用户手工执行。

---

## 14. tests/test_sqlite_cache.cpp — 新增

测试：

```text
SQLite :memory:
私聊 insert/query
群聊 insert/query
stats
offline flag
```

---

## 15. tests/test_redis_client.cpp — 新增

测试内部启动一个最小 fake Redis TCP server。

用真正 hiredis 连接它，验证：

```text
SELECT
PING
SET NX
GET
EVAL refresh
EVAL delete
HINCRBY
EXPIRE
HMGET
```

所以不只是“RedisClient 能编译”。

---

## 16. 没有改的核心

### MySQL schema

v8.1 没有新增服务端 MySQL migration。

### Reactor 核心

本版没有修改：

```text
Buffer
Channel
Poller
EventLoop
Acceptor
TcpConnection
TcpServer
EventLoopThreadPool
```

### 群组命令

v8.0 群组协议保持兼容。

### 文件传输

仍留到下一版本。
