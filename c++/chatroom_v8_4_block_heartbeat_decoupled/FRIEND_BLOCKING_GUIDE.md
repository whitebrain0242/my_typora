# v8.4 好友消息屏蔽设计

## 命令

```text
BLOCK_FRIEND <username>
UNBLOCK_FRIEND <username>
BLOCKED_FRIENDS
```

## 屏蔽范围

会屏蔽：

```text
MSG 私聊文字
SEND_FILE 私聊文件
离线私聊文字投递
离线私聊文件投递
```

不会屏蔽：

```text
SAY 公共聊天
GROUP_MSG 群聊天
群文件
好友关系本身
旧 HISTORY_PRIVATE
在线/离线好友提示
```

## 持久化

MySQL：

```text
friend_blocks
```

方向性：

```text
Alice blocks Bob
!=
Bob blocks Alice
```

## 新消息

被屏蔽者发送新私聊/私聊文件：

```text
permission check
↓
BlockedByRecipient
↓
reject
```

不写新的 message/file delivery。

## 已经存在的离线消息

如果消息在 block 前已经存在：

```text
仍保存
block active 时查询过滤
unblock 后可以 PENDING
```

这样不会把“未交付”伪装成“已交付”。

## 删除好友

`remove_friendship()` 现在使用事务同时删除双方的 block rows。

原因：

```text
block 的定义前提 = 当前仍是好友
```


## Redis 未读缓存说明

`BLOCK_FRIEND` 的可靠过滤由 MySQL `friend_blocks` + delivery 查询决定。

Redis 未读计数仍是缓存，因此如果一个 direct item 在被 block 之前已经进入 pending，Redis 的聚合数字可能暂时仍包含它；这不会导致消息被投递。真正是否可投递以 MySQL 查询为准，实际完成投递后 Redis 计数再递减。
