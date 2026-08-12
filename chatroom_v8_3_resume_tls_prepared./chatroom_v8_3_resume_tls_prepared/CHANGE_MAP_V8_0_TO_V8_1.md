# v8.0 → v8.1 精确文件变更清单

对比基线：

```text
chatroom_v8_0_reactor_groups_offline.zip
```

结果：

```text
修改 11 个文件
新增 9 个文件
删除 0 个文件
```

## 修改的 11 个文件

### 1. CMakeLists.txt

新增：

```text
hiredis
sqlite3
```

新增源码：

```text
src/redis_client.cpp
src/sqlite_client.cpp
```

新增测试：

```text
tests/test_redis_client.cpp
tests/test_sqlite_cache.cpp
```

### 2. README.md

升级为 v8.1 使用说明，加入：

```text
Redis 服务端职责
SQLite 客户端职责
依赖安装
Redis 配置
新运行命令
LOCAL_* 本地命令
调试方法
```

### 3. VERSION_NOTES.md

更新为 v8.1 版本记录。

### 4. include/chat_server.hpp

构造函数新增：

```cpp
RedisClient&
server_instance_id
presence_ttl_seconds
```

新增 Redis 相关成员：

```text
redis_
server_instance_id_
presence_ttl_seconds_
presence_refresh_thread_
presence_wait_cv_
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

### 5. include/config.hpp

新增：

```cpp
RedisConfig
load_redis_config()
```

### 6. include/integration/redis_client.hpp

v8.0 的占位接口改为真正的 hiredis 封装。

主要 API：

```text
connect
ping
claim_presence
refresh_presence_if_owned
remove_presence_if_owned
presence_owner
adjust_unread
unread_counts
```

### 7. include/integration/sqlite_client.hpp

v8.0 的占位接口改为真实 SQLite 封装。

新增：

```text
LocalPrivateMessage
LocalGroupMessage
LocalCacheStats
SqliteClient
```

### 8. src/chat_server.cpp

修改部分：

```text
constructor / destructor
on_connection
handle_login
handle_logout
handle_private_message
handle_group_message
handle_pending
deliver_pending_messages
```

新增：

```text
Redis presence 维护线程
Redis 登录占用
Redis 退出清理
Redis 私聊未读 +1/-1
Redis 群聊未读 +1/-1
Redis 未读摘要
```

### 9. src/client.cpp

原来：

```text
recv chunk → 直接 cout.write
```

现在：

```text
recv chunk
→ 累积 server_buffer
→ 按 \n 拆完整行
→ 打印
→ 识别登录账号
→ 解析私聊/群聊行
→ 写 SQLite
```

新增客户端本地命令：

```text
LOCAL_HELP
LOCAL_DB
LOCAL_STATS
LOCAL_HISTORY_PRIVATE
LOCAL_HISTORY_GROUP
```

新增第三个运行参数：

```text
SQLite database path
```

### 10. src/config.cpp

新增通用：

```text
load_key_values()
```

新增 Redis 配置解析和范围验证。

### 11. src/server_main.cpp

新增：

```text
RedisConfig
RedisClient
Redis connect
Redis ping
server_instance_id
```

新推荐启动方式：

```bash
./build/chat_server 9000 config/mysql.conf config/redis.conf 4
```

仍兼容 v8.0：

```bash
./build/chat_server 9000 config/mysql.conf 8
```

---

## 新增的 9 个文件

```text
MIGRATION_FROM_V8_0.md
REDIS_GUIDE.md
SQLITE_GUIDE.md
config/redis.conf.example
sql/client_sqlite_schema.sql
src/redis_client.cpp
src/sqlite_client.cpp
tests/test_redis_client.cpp
tests/test_sqlite_cache.cpp
```

---

## 删除文件

```text
0
```

本版没有删除 v8.0 功能文件。

---

## 本版没有动的关键部分

### Reactor 网络核心没有改

```text
include/minimuduo/net/*
src/minimuduo/net/*
```

因此：

```text
MainReactor / SubReactor
EventLoop
Channel
Poller
TcpConnection
TcpServer
eventfd
```

保持 v8.0 架构。

### MySQL schema 没有新增 migration

仍使用 v8.0 的：

```text
sql/004_create_groups_and_offline_delivery.sql
```

### 群组命令没有删除或改名

群管理、群聊、群历史、离线群消息保持兼容。

### 文件传输仍未实现

继续留到下一版本。
