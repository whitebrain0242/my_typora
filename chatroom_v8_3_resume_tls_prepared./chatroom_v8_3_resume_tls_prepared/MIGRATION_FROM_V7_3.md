# 从真实 v7.3 到 v8.0 的修改说明

## 1. 原 v7.3

上传的 v7.3 中：

```text
ChatServer
├── listen_fd_
├── epoll_fd_
├── clients_
├── online_users_
├── accept_new_clients()
├── handle_client_read()
├── handle_client_write()
├── epoll_wait()
└── 所有 handle_* 业务函数
```

也就是说网络层和业务层混在同一个类中。

只有一个线程和一个 epoll：

```text
listen fd
client fd
client fd
client fd
    ↓
同一个 epoll
    ↓
同一个线程
```

---

## 2. 本版拆分

旧职责：

```text
ChatServer::create_listen_socket
ChatServer::accept_new_clients
ChatServer::update_epoll_events
ChatServer::handle_client_read
ChatServer::handle_client_write
```

移动到了：

| v7.3 逻辑 | v8.0 |
|---|---|
| `epoll_create1 / epoll_wait` | `Poller + EventLoop` |
| socket 事件状态 | `Channel` |
| accept | `Acceptor` |
| recv/send 缓冲 | `TcpConnection + Buffer` |
| 客户端连接生命周期 | `TcpConnection` |
| 服务器连接表 | `TcpServer` |
| worker 创建 | `EventLoopThreadPool` |
| 跨线程任务 | `runInLoop / queueInLoop` |
| 跨线程唤醒 | `eventfd` |

---

## 3. ChatServer 现在只做业务适配

构造时：

```cpp
tcp_server_.setConnectionCallback(...);
tcp_server_.setMessageCallback(...);
```

连接建立：

```cpp
connection->setContext(
    std::make_shared<ClientSession>()
);
```

收到数据：

```cpp
while (const char* eol = buffer->findEOL()) {
    // 按 '\n' 切出一行
    const Command command = parse_command(line);
    handle_command(connection, session, command);
}
```

因此原来的：

```text
parse_command
command.raw_arguments
handle_register
handle_login
handle_public_message
handle_private_message
好友函数
历史查询
```

都仍然存在。

---

## 4. fd → TcpConnectionPtr

v7.3：

```cpp
void handle_private_message(
    int client_fd,
    const std::string& arguments
);
```

本版：

```cpp
void handle_private_message(
    const TcpConnectionPtr& connection,
    const ClientSession& session,
    const std::string& arguments
);
```

业务逻辑没有再直接 `send(fd, ...)`，统一：

```cpp
connection->send(...);
```

---

## 5. 输出缓冲不再属于 ChatServer

v7.3：

```cpp
ClientSession {
    int fd;
    std::string in_buffer;
    std::string out_buffer;
}
```

本版：

```cpp
ClientSession {
    bool logged_in;
    std::string username;
}
```

网络状态放到：

```text
TcpConnection
├── socket
├── Channel
├── inputBuffer
└── outputBuffer
```

这样 `ClientSession` 只保留业务状态。

---

## 6. 在线用户表为什么改成 weak_ptr

v7.3：

```cpp
unordered_map<string, int> online_users_;
```

本版：

```cpp
unordered_map<
    string,
    weak_ptr<TcpConnection>
> online_users_;
```

`TcpServer` 持有连接的 `shared_ptr`。

在线业务索引只持有 `weak_ptr`，避免因为“在线表”意外延长已经关闭连接的生命周期。

---

## 7. Channel 生命周期

```text
TcpServer
  owns shared_ptr<TcpConnection>

TcpConnection
  owns unique_ptr<Channel>

Channel::tie(shared_from_this())
  stores weak owner guard
```

事件分发时先锁住 owner，防止 callback 执行到一半连接对象被销毁。

---

## 8. 主从 Reactor

MainReactor：

```text
main thread
EventLoop
Poller(epoll)
Acceptor Channel
```

SubReactor：

```text
worker thread
EventLoop
Poller(epoll)
eventfd Channel
many TcpConnection Channels
```

`TcpServer::newConnection()` 在 MainReactor：

```cpp
EventLoop* io_loop =
    thread_pool_->getNextLoop();
```

然后：

```cpp
io_loop->runInLoop(
    [connection] {
        connection->connectEstablished();
    }
);
```

客户端 socket 从此由该 SubReactor 管理。

---

## 9. 新增并发保护

v7.3 的业务天然串行。

多 SubReactor 后以下数据会被并发访问。

### 在线表

```cpp
std::mutex online_mutex_;
```

### 好友复合操作

```cpp
std::mutex friend_operation_mutex_;
```

### 群权限/成员操作

```cpp
std::mutex group_operation_mutex_;
```

### 单 MySQL 连接

```cpp
MySqlDatabase::mutex_
```

---

## 10. 私聊离线投递变化

旧逻辑：

```text
好友离线
→ 拒绝发送
```

新逻辑：

```text
好友关系检查
→ 保存 messages
→ 保存 private_message_deliveries
→ 在线则实时发送，输出缓冲排空后标记 delivered
→ 离线则保持 pending
→ LOGIN/PENDING 时投递
```

原 `HISTORY_PRIVATE` 不受影响，仍从 `messages` 查询。

---

## 11. 群功能新增层次

关系数据：

```text
chat_groups
group_members
group_join_requests
```

消息：

```text
GroupMessagePayload
→ serialize_group_message
→ group_messages
```

离线投递：

```text
group_message_deliveries
```

---

## 12. 客户端

`src/client.cpp` 没有为了群功能重新设计。

因为它本来就是：

```text
stdin 一行
→ 原样发给服务器

服务器返回
→ 原样显示
```

所以新命令可以直接输入，旧运行方式保持兼容。
