# v8.4 Database / Cache Ownership Plan

v8.4 有三个数据层：

```text
MySQL   = 服务端可靠持久化 / 权限真相
Redis   = 服务端快速 presence + unread cache
SQLite  = 客户端本地历史 / 文件 / 断点状态
```

# MySQL

## users

```text
账号
PBKDF2 encoded password hash
```

## friend_requests

```text
好友申请
```

## friendships

```text
双向好友关系
```

## friend_blocks — v8.4

```text
blocker_username
blocked_username
```

是有方向的 direct-delivery policy。

## friend_events

官方 Protobuf BLOB：

```text
FriendEventPayload
```

## messages

公共/私聊历史：

```text
PUBLIC
PRIVATE
ChatMessagePayload protobuf
```

## private_message_deliveries

私聊每条消息的离线 delivery 状态。

v8.4 查询会根据：

```text
friend_blocks
```

过滤当前不允许投递的 sender。

## chat_groups

群基本信息。

## group_members

```text
Owner/Admin/Member
```

## group_join_requests

持久群申请。

## group_messages

群消息 + `GroupMessagePayload` Protobuf。

## group_message_deliveries

群消息 per-recipient delivery。

## file_transfers

已经完成上传并由服务端保存的文件 metadata。

metadata BLOB 使用官方：

```text
FileTransferMetadata
```

## file_transfer_deliveries

文件 per-recipient delivery。

v8.4：

```text
PRIVATE file
```

投递受 `friend_blocks` 控制；

```text
GROUP file
```

不受好友 direct block 控制。

---

# Redis

Redis 不保存可靠业务关系。

当前：

```text
presence
private unread
group unread
private_file unread
group_file unread
server instance ownership
```

Redis 不保存：

```text
friendship truth
friend block truth
message delivery truth
file delivery truth
```

这些都属于 MySQL。

---

# SQLite

客户端：

```text
private_messages
group_messages
file_transfers
pending_uploads
partial_downloads
```

`pending_uploads`：

```text
客户端待恢复上传任务
```

`partial_downloads`：

```text
客户端待恢复下载 identity
```

真实下载 offset 取：

```text
.part actual file size
```

而不是在 SQLite 再维护一份 offset。

---

# v8.4 Migration

v8.3 → v8.4 只有 MySQL 新表：

```text
sql/006_create_friend_blocks.sql
```

执行：

```bash
sudo mysql chatroom \
  < sql/006_create_friend_blocks.sql
```

SQLite/Redis 不需要 schema migration。
