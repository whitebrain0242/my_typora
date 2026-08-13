# chatroom v8.4 — 服务端完整架构设计

## 1. 总体目标

v8.4 服务端继续保留：

```text
Main/Sub Reactor
TLS
官方 Protobuf
MySQL
Redis
群组
好友
离线消息
在线/离线文件
断点续传
```

新增：

```text
好友直聊屏蔽
TCP/TLS 心跳检测
```

同时把原来集中在 `ChatServer` 中的基础职责拆出。

最终结构：

```text
                    ┌──────────────────────┐
                    │     MainReactor      │
                    │ main thread + epoll  │
                    └──────────┬───────────┘
                               │ accept
                               ▼
                       TcpServer / Acceptor
                               │
                       round-robin assign
          ┌────────────────────┼────────────────────┐
          ▼                    ▼                    ▼
    SubReactor 0         SubReactor 1         SubReactor N
    epoll+eventfd        epoll+eventfd        epoll+eventfd
          │                    │                    │
          └────────── TcpConnection + TLS ─────────┘
                               │
                               ▼
                        ServerCommandRouter
                               │
                               ▼
                           ChatServer
                       orchestration facade
          ┌───────────────┬────┼─────────┬────────────────┐
          ▼               ▼    ▼         ▼                ▼
 OnlineUserRegistry  DirectPolicy  FileTransfer    Redis Presence
                          │          Service
                          ▼             │
                     MySqlDatabase      └→ disk workers
                          │
                          ▼
                    MySQL durable data

Independent liveness path:
HeartbeatManager thread
      │
      ├── PING connection
      ├── track last activity
      └── forceClose timeout
```

---

# 2. 网络层

目录：

```text
include/minimuduo/net/
src/minimuduo/net/
```

核心：

```text
EventLoop
Poller
Channel
Buffer
Acceptor
TcpServer
TcpConnection
EventLoopThread
EventLoopThreadPool
TlsContext
SocketOptions
```

---

# 3. MainReactor

MainReactor：

```text
main thread
+
one epoll
+
Acceptor
```

只承担：

```text
listen
accept
worker EventLoop allocation
TcpConnection creation/lifecycle ownership
```

不做：

```text
LOGIN
MySQL query
FILE_CHUNK
GROUP_MSG
SHA
```

---

# 4. SubReactor

每个 worker：

```text
1 thread
+
1 EventLoop
+
1 epoll
+
1 eventfd
```

连接固定归属：

```text
connection accepted
↓
round-robin worker
↓
该 socket 之后的 SSL_read/SSL_write
都在这个 SubReactor
```

跨线程：

```text
connection->send()
↓
queueInLoop
↓
eventfd
↓
owner SubReactor
```

---

# 5. TLS

`TcpConnection` 的应用 I/O：

```text
epoll readable
↓
SSL_read
↓
Buffer
↓
ChatServer::on_message
```

发送：

```text
ChatServer / file worker / heartbeat thread
↓
TcpConnection::send
↓
owner EventLoop
↓
SSL_write
```

TLS handshake：

```text
TCP accepted
↓
SubReactor
↓
nonblocking SSL_accept
↓
WANT_READ / WANT_WRITE
↓
epoll
↓
handshake complete
↓
ChatServer::on_connection
```

---

# 6. TCP keepalive + 应用层心跳

v8.4 使用双层策略。

## 6.1 Linux TCP keepalive

新模块：

```text
include/minimuduo/net/SocketOptions.hpp
src/minimuduo/net/SocketOptions.cpp
```

接受连接时设置：

```text
SO_KEEPALIVE = 1
TCP_KEEPIDLE = 60
TCP_KEEPINTVL = 15
TCP_KEEPCNT = 3
```

客户端 TCP 也设置相同 fallback。

OS keepalive 的价值：

```text
内核层检测长时间半开连接
```

但不同平台默认行为可能不同，所以它不是唯一机制。

## 6.2 应用层 heartbeat

新模块：

```text
include/server/heartbeat_manager.hpp
src/server/heartbeat_manager.cpp
```

默认：

```text
PING interval = 20s
server timeout = 60s
scan interval = 1s
```

协议：

```text
Server → PING <nonce>
Client → PONG <nonce>
```

### 为什么仍叫 TCP 心跳

因为它是在长期 TCP/TLS connection 上用于：

```text
连接存活检测
半开连接清理
NAT/链路异常发现
```

但具体 heartbeat frame 位于 TLS application data 中，而不是裸 TCP header。

这比只依赖系统 TCP keepalive 更容易得到确定的 20/60 秒行为。

---

# 7. HeartbeatManager 线程模型

HeartbeatManager 有一个独立轻量线程。

它不直接操作 socket fd。

它只调用：

```cpp
connection->send(...)
connection->forceClose()
```

而 `TcpConnection` 会把操作投递回 owner EventLoop。

因此：

```text
Heartbeat thread
不会直接 SSL_write
不会直接 epoll_ctl connection fd
```

线程归属仍然正确。

## 7.1 锁策略

HeartbeatManager mutex 只保护：

```text
connection weak_ptr
last_activity
last_ping
last_nonce
```

扫描时先收集：

```text
PingAction
TimedOutConnection
```

释放 mutex 后才：

```text
send
forceClose
```

避免：

```text
heartbeat mutex
→ EventLoop callback
→ 业务代码
```

形成长锁链。

---

# 8. ServerCommandRouter

新模块：

```text
include/server/command_router.hpp
src/server/command_router.cpp
src/server/chat_server_routes.cpp
```

旧结构：

```cpp
if (HELP) ...
else if (LOGIN) ...
else if (MSG) ...
else if (...)
```

v8.4：

```text
command name
↓
unordered_map<string, Handler>
↓
handler
```

`ChatServer::handle_command` 只剩：

```text
router.dispatch
or unknown-command error
```

路由注册也被单独移到：

```text
chat_server_routes.cpp
```

因此：

```text
协议命令表
```

和：

```text
具体业务实现
```

不再混在一个函数里。

---

# 9. ClientSession

新文件：

```text
include/server/client_session.hpp
```

从 `chat_server.hpp` 中移出。

它只保存“一个 TCP connection 对应的业务 session 状态”：

```text
logged_in
username
active resumable upload
file deliveries in progress
offered file metadata
```

网络 connection 的 lifetime 仍由：

```text
TcpServer / shared_ptr<TcpConnection>
```

管理。

---

# 10. OnlineUserRegistry

新模块：

```text
include/server/online_user_registry.hpp
src/server/online_user_registry.cpp
```

旧：

```text
ChatServer
├── online_mutex_
└── online_users_
```

新：

```text
OnlineUserRegistry
├── add
├── remove
├── find
├── is_online
├── usernames
└── connections
```

关键规则：

```text
mutex 内只管理 weak_ptr/map
mutex 外才 connection->send
```

例如广播：

```text
registry.connections()
↓
释放 registry mutex
↓
for connection:
    send()
```

避免在线表锁跨网络发送。

---

# 11. DirectMessagePolicy

新模块：

```text
include/server/direct_message_policy.hpp
src/server/direct_message_policy.cpp
```

统一私聊文字和私聊文件的准入规则：

```text
recipient exists?
↓
friends?
↓
recipient blocks sender?
↓
Allowed
```

结果：

```text
Allowed
TargetMissing
NotFriends
BlockedByRecipient
DatabaseError
```

这样：

```text
MSG
FILE_BEGIN_PRIVATE
FILE_END private re-check
```

不会各自写一套不同权限判断。

---

# 12. 好友消息屏蔽

新命令：

```text
BLOCK_FRIEND <username>
UNBLOCK_FRIEND <username>
BLOCKED_FRIENDS
```

## 12.1 语义

屏蔽的是：

```text
direct private text
direct private file
```

不改变：

```text
friendship
old HISTORY_PRIVATE
public SAY
group chat
group files
group membership
friend online/offline presence
```

因此这是：

```text
message delivery policy
```

不是：

```text
remove friend
```

---

# 13. friend_blocks 表

MySQL 新表：

```text
friend_blocks
```

主键：

```text
(blocker_username, blocked_username)
```

关系有方向。

例如：

```text
alice blocks bob
```

不等于：

```text
bob blocks alice
```

SQL：

```text
sql/006_create_friend_blocks.sql
```

---

# 14. 屏蔽发送流程

Bob：

```text
MSG alice hello
```

服务端：

```text
ServerCommandRouter
↓
handle_private_message
↓
friend_operation_mutex
↓
DirectMessagePolicy
├── users
├── friendships
└── friend_blocks
↓
BlockedByRecipient?
├── yes → reject before message insert
└── no  → MySQL message + delivery
```

因此被屏蔽后发送的新消息：

```text
不会进入 messages
不会增加 delivery row
不会进入 Alice 的离线队列
```

---

# 15. 屏蔽和已有离线消息

如果：

```text
Bob 先给 Alice 发消息
Alice 当时离线
↓
message pending
↓
Alice 后来 BLOCK_FRIEND bob
```

v8.4 的选择是：

```text
消息仍然保存在 MySQL
但 BLOCK active 时不投递
```

查询：

```text
pending_private_messages
```

使用：

```sql
NOT EXISTS (
    SELECT 1
    FROM friend_blocks ...
)
```

解除屏蔽后：

```text
UNBLOCK_FRIEND bob
PENDING
```

旧 pending 可以继续投递。

这个设计避免把：

```text
blocked
```

伪装成：

```text
delivered
```

---

# 16. 私聊文件屏蔽

私聊文件有三个检查点：

```text
1 FILE_BEGIN_PRIVATE
2 FILE_END
3 live/pending delivery before FILE_OFFER
```

为什么 FILE_END 要再次检查？

因为：

```text
开始上传时允许
↓
上传 20MB
↓
期间 recipient BLOCK sender
↓
FILE_END
```

如果只在 FILE_BEGIN 检查，就会绕过新 block。

所以 FILE_END 会重新评估 DirectMessagePolicy。

---

# 17. ChatServer 角色

v8.4 中 `ChatServer` 仍然是业务入口，但更像 orchestration facade。

它协调：

```text
route
session
database
redis
file service
registry
policy
heartbeat
```

而不再直接拥有：

```text
online map implementation
heartbeat timer implementation
command lookup implementation
direct-message policy implementation
```

新增分拆文件：

```text
src/server/chat_server_routes.cpp
src/server/chat_server_blocking.cpp
src/server/chat_server_heartbeat.cpp
```

---

# 18. MySqlDatabase

继续使用 v8.3 的标准：

```text
MYSQL_STMT
?
MYSQL_BIND
```

新增 block CRUD 也完全使用 Prepared Statement：

```text
is_friend_blocked
add_friend_block
remove_friend_block
list_blocked_friends
```

没有恢复 SQL 字符串拼接。

## 18.1 MySQL 并发模型

仍然是：

```text
one MYSQL*
+
MySqlDatabase mutex
+
synchronous DB calls
```

所以：

```text
Prepared Statement
≠
async/high-throughput DB pool
```

这是当前架构明确保留的性能限制。

---

# 19. Redis

Redis 继续负责：

```text
presence
unread counters
server instance ownership
```

MySQL 继续负责：

```text
durable source of truth
```

屏蔽关系只放 MySQL。

理由：

```text
block 是权限/持久关系
不能因为 Redis 丢 key 就失效
```

未来如果性能需要，可以把 block set 缓存在 Redis，但 MySQL 仍应是真相源。

---

# 20. FileTransferService

仍使用独立 disk worker threads。

```text
SubReactor
↓
业务判断
↓
FileTransferService task
↓
disk read
↓
TcpConnection::send
↓
eventfd
↓
owner SubReactor
```

File worker 不直接操作 SSL fd。

---

# 21. 服务端线程总览

假设：

```text
worker_threads = 4
```

大致线程：

```text
Thread 0  MainReactor
Thread 1  SubReactor 0
Thread 2  SubReactor 1
Thread 3  SubReactor 2
Thread 4  SubReactor 3
Thread 5  Redis presence refresh
Thread 6  HeartbeatManager
Thread 7  File worker 0
Thread 8  File worker 1
```

MySQL 没有独立 worker thread。

它在调用它的业务线程中同步执行，并由 mutex 串行化。

---

# 22. 服务端锁边界

## OnlineUserRegistry mutex

保护：

```text
username → weak connection
```

不在锁内 send。

## HeartbeatManager mutex

保护：

```text
heartbeat entry
```

不在锁内 forceClose/send。

## friend_operation_mutex_

保护 compound friend/direct-policy 操作，例如：

```text
BLOCK_FRIEND
MSG policy + persistence ordering
private-file permission checks
```

## group_operation_mutex_

保护：

```text
group membership
role/admin
join request
```

复合修改。

## MySqlDatabase mutex

保护单一：

```text
MYSQL*
```

---

# 23. 心跳完整流程

```text
Heartbeat thread
every 1s scan
      │
      ├── idle < 20s
      │       nothing
      │
      ├── >= 20s since last ping
      │       PING nonce
      │
      └── >= 60s since last inbound activity
              forceClose
```

客户端：

```text
TLS line PING nonce
↓
ClientHeartbeat
↓
PONG nonce
```

服务端：

```text
on_message
↓
note_activity
↓
route PONG
↓
HeartbeatManager::note_pong
```

任何有效入站 TCP/TLS application bytes 也视为活跃。

所以大型上传期间：

```text
FILE_CHUNK
FILE_CHUNK
...
```

本身就不断刷新连接活跃时间，不会因为客户端暂时没处理 PING 而误杀。

---

# 24. 故障语义

## 客户端拔网线 / 半开

```text
no inbound activity
↓
60s heartbeat timeout
↓
forceClose
↓
ChatServer disconnect callback
↓
online registry cleanup
↓
Redis presence cleanup
↓
broadcast offline
```

## Heartbeat thread停止

应用 TCP connection 仍有 Linux keepalive fallback。

## Redis 故障

不决定 TCP connection 存活。

## MySQL 故障

Heartbeat 仍可工作，因为它不访问 DB。

这正是解耦后的好处。

---

# 25. 服务端依赖方向

推荐理解为：

```text
net
↑
TcpServer/TcpConnection
↑
ChatServer orchestration
├── Router
├── Registry
├── Heartbeat
├── DirectPolicy
├── FileTransferService
├── MySqlDatabase
└── RedisClient
```

不希望出现：

```text
MySqlDatabase → TcpConnection
RedisClient → ChatServer
HeartbeatManager → MySQL
SocketOptions → group logic
```

---

# 26. 后续扩展位置

如果下一个版本增加：

```text
消息已读回执
消息撤回
多端登录
好友分组
服务端推送
```

可以新增：

```text
server/message_receipt_service
server/session_registry
server/notification_service
server/contact_service
```

不需要再次把所有逻辑塞进 `ChatServer`。

---

# 27. 一句话理解服务端

```text
Reactor/TLS 负责连接，
Router 负责命令到处理器，
ChatServer 负责编排，
Registry 负责在线关系，
Heartbeat 负责存活，
DirectPolicy 负责私聊权限，
MySQL 负责可靠状态，
Redis 负责快速状态，
FileTransferService 负责文件 I/O。
```
