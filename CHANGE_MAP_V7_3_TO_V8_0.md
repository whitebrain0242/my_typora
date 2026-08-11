# v7.3 → v8.0 文件级修改清单

## 原文件：保持业务/协议兼容，未改或基本未改

```text
src/client.cpp
src/protocol.cpp
include/protocol.hpp
src/config.cpp
include/config.hpp
src/password.cpp
include/password.hpp
sql/001_create_users.sql
sql/002_create_friends_and_events.sql
sql/003_create_messages.sql
proto/friend_event.proto
proto/chat_message.proto
```

## 原文件：重点修改

### CMakeLists.txt

新增：

- Threads
- minimuduo 静态库
- Reactor 测试
- Buffer 测试

### include/chat_server.hpp / src/chat_server.cpp

删除旧网络职责：

- listen fd
- epoll fd
- client fd map
- input/output socket buffer
- accept
- recv
- send
- epoll_ctl
- epoll_wait

增加：

- `on_connection`
- `on_message`
- `TcpConnectionPtr`
- `ClientSession` context
- 在线用户 weak_ptr 表及并发保护
- 离线私聊投递
- 全部群组命令
- 离线群消息
- `PENDING`

### include/mysql_database.hpp / src/mysql_database.cpp

保留所有 v7.3 API，并增加：

- MySQL 单连接跨 SubReactor 串行保护
- `private_message_deliveries`
- 群基本信息
- 群成员/角色
- 加群申请
- 群消息
- 群消息 delivery
- pending/mark-delivered API

### include/proto_codec.hpp / src/proto_codec.cpp

原好友/聊天 codec 保留。

新增：

```text
GroupMessagePayload
serialize_group_message
parse_group_message
```

### src/server_main.cpp

v7.3：

```text
MySqlDatabase
→ ChatServer(port, db)
→ initialize
→ run(epoll)
```

v8.0：

```text
MySqlDatabase
→ Main EventLoop
→ TcpServer
→ EventLoopThreadPool(4)
→ ChatServer callbacks
→ mainLoop.loop()
```

### tests/test_proto.cpp

新增 GroupMessagePayload round-trip。

---

## 新增：网络层

```text
include/minimuduo/net/NonCopyable.hpp
include/minimuduo/net/Callbacks.hpp
include/minimuduo/net/Buffer.hpp
include/minimuduo/net/Channel.hpp
include/minimuduo/net/Poller.hpp
include/minimuduo/net/EventLoop.hpp
include/minimuduo/net/EventLoopThread.hpp
include/minimuduo/net/EventLoopThreadPool.hpp
include/minimuduo/net/Acceptor.hpp
include/minimuduo/net/TcpConnection.hpp
include/minimuduo/net/TcpServer.hpp

src/minimuduo/net/Buffer.cpp
src/minimuduo/net/Channel.cpp
src/minimuduo/net/Poller.cpp
src/minimuduo/net/EventLoop.cpp
src/minimuduo/net/EventLoopThread.cpp
src/minimuduo/net/EventLoopThreadPool.cpp
src/minimuduo/net/Acceptor.cpp
src/minimuduo/net/TcpConnection.cpp
src/minimuduo/net/TcpServer.cpp
```

---

## 新增：群/离线数据库

```text
sql/004_create_groups_and_offline_delivery.sql
MYSQL_RUN_THIS_V8_0.sql
```

---

## 新增：Protobuf schema

```text
proto/group_message.proto
```

---

## 新增：后续接口

```text
include/integration/redis_client.hpp
include/integration/sqlite_client.hpp
include/integration/file_transfer.hpp
```

---

## 新增：验收测试

```text
tests/test_buffer.cpp
tests/test_reactor_smoke.cpp
tests/test_reactor_distribution.cpp
tests/test_cross_thread_send.cpp
```
