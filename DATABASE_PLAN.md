# v8.0 Database Plan

## 原 v7.3 表

```text
users
friend_requests
friendships
friend_events
messages
```

保持兼容。

## 新增

### private_message_deliveries

解决：

```text
好友离线时仍可 MSG
```

### chat_groups

群基本信息和 owner。

### group_members

```text
role=1 owner
role=2 admin
role=3 member
```

### group_join_requests

保存等待 owner/admin 审批的申请。

### group_messages

保存 Protobuf group payload。

### group_message_deliveries

群消息按用户维护独立投递状态。

## 为什么不把离线消息只放内存

如果使用：

```cpp
unordered_map<string, vector<Message>>
```

服务端一重启离线消息就消失。

本版把待投递状态放 MySQL，满足：

```text
服务器重启
→ 用户之后登录
→ 消息仍可投递
```

## Redis 下一步

Redis 接入后可以承担：

```text
presence
Pub/Sub
高速 notification queue
```

但 MySQL 仍作为最终持久化记录。
