# v8.4 新增代码导读

这份文档只讲“v8.4 为什么这样改、核心新增代码在哪里”，用于和 v8.3 对照学习。

# 一、好友消息屏蔽

## 1. MySQL 新关系

新增表：

```text
friend_blocks(blocker_username, blocked_username)
```

它是有方向的：

```text
Alice blocks Bob
!=
Bob blocks Alice
```

新增数据库接口：

```cpp
bool is_friend_blocked(...);
bool add_friend_block(...);
bool remove_friend_block(...);
bool list_blocked_friends(...);
```

全部继续走 v8.3 的 Prepared Statement 封装：

```text
MYSQL_STMT
?
MYSQL_BIND
```

## 2. DirectMessagePolicy

新增：

```text
include/server/direct_message_policy.hpp
src/server/direct_message_policy.cpp
```

统一判断：

```text
recipient exists
↓
friendship exists
↓
recipient blocks sender?
↓
Allowed
```

这样 `MSG` 和私聊文件不再各写一套权限逻辑。

## 3. BLOCK 命令

实现位于：

```text
src/server/chat_server_blocking.cpp
```

命令：

```text
BLOCK_FRIEND bob
UNBLOCK_FRIEND bob
BLOCKED_FRIENDS
```

Block 不删除好友关系。

它只影响 direct channel：

```text
MSG
SEND_FILE
offline private message delivery
offline private file delivery
```

## 4. 已存在的 pending 数据

旧 pending 不删除、不假装 delivered。

MySQL 查询增加：

```sql
NOT EXISTS (
    SELECT 1
    FROM friend_blocks ...
)
```

因此：

```text
blocked → 暂停投递
unblocked + PENDING → 可以继续投递
```

# 二、TCP/TLS 心跳

## 1. HeartbeatManager

新增：

```text
include/server/heartbeat_manager.hpp
src/server/heartbeat_manager.cpp
```

默认：

```text
PING 20s
timeout 60s
scan 1s
```

它只维护：

```text
weak connection
last_activity
last_ping
last_nonce
```

扫描时先在锁内得到 action，释放锁以后再：

```cpp
connection->send(...)
connection->forceClose()
```

不会持 heartbeat mutex 做网络操作。

## 2. PING/PONG

服务端协议入口：

```text
src/server/chat_server_heartbeat.cpp
```

客户端：

```text
src/client/client_heartbeat.cpp
```

协议：

```text
PING 123
PONG 123
```

这些行位于 TLS application data 中。

## 3. TCP keepalive

新增：

```text
SocketOptions.hpp/.cpp
```

服务端 accepted socket 和客户端 socket 都设置：

```text
SO_KEEPALIVE
TCP_KEEPIDLE=60
TCP_KEEPINTVL=15
TCP_KEEPCNT=3
```

应用 PING/PONG 是主要的确定性检测，kernel keepalive 是第二层 fallback。

# 三、服务端解耦

## 1. Command Router

旧：

```cpp
if (...) ...
else if (...) ...
else if (...) ...
```

新：

```text
ServerCommandRouter
command name
→ handler
```

路由定义集中到：

```text
src/server/chat_server_routes.cpp
```

业务实现不再和命令映射混在一起。

## 2. OnlineUserRegistry

原来的：

```text
ChatServer::online_mutex_
ChatServer::online_users_
```

移动到：

```text
OnlineUserRegistry
```

提供：

```text
add
remove
find
is_online
usernames
connections
```

发送网络消息不在 Registry mutex 内执行。

## 3. ClientSession

从 `chat_server.hpp` 移到：

```text
include/server/client_session.hpp
```

它现在更明确地只是“connection 对应的业务 session data”。

## 4. ChatServer

ChatServer 仍然是 orchestration facade。

它负责协调：

```text
Registry
Router
Policy
Heartbeat
MySQL
Redis
FileTransferService
```

而不是自己实现每个基础设施细节。

# 四、客户端解耦

## 1. src/client.cpp

现在只做：

```text
argv
↓
ClientAppConfig
↓
ClientApp::run
```

它不再知道：

```text
SSL*
FILE_DATA
SQLite SQL
PING/PONG
poll implementation
```

## 2. ClientApp

负责：

```text
初始化
poll
line framing
模块分发
login/logout state
```

服务端一行的处理顺序：

```text
Heartbeat
↓
File protocol
↓
Normal text
↓
SQLite message cache
```

## 3. TlsClientTransport

唯一拥有：

```text
socket fd
SSL*
TlsClientContext
peer identity
```

对上只暴露：

```text
connect
send
receive
fd
pending
shutdown
```

## 4. FileTransfer

所有 v8.3 文件/断点续传行为移动到：

```text
client_file_transfer.cpp
```

协议和语义不变。

## 5. Local Commands

`LOCAL_*`、`SEND_FILE`、`RESUME_UPLOADS` 移到：

```text
client_local_commands.cpp
```

## 6. Message Cache

服务端文本：

```text
[#12] [private from bob] hello
```

解析为 SQLite record 的逻辑移到：

```text
client_message_cache.cpp
```

# 五、为什么这种拆法适合后续版本

以后加：

```text
已读回执
消息撤回
多端登录
文件进度
通知中心
好友备注/分组
```

可以新增 service/controller，而不是继续扩大：

```text
chat_server.cpp
client.cpp
```

完整依赖关系请看：

```text
CLIENT_ARCHITECTURE_V8_4.md
SERVER_ARCHITECTURE_V8_4.md
```
