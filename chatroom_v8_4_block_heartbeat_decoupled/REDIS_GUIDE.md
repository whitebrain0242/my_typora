# Redis Guide — v8.4

v8.1 已有：

```text
presence
private message unread
group message unread
```

v8.2 新增：

```text
private_file
group_file
```

Hash 示例：

```text
chatroom:unread:bob
├── private
├── group
├── private_file
└── group_file
```

文件上传成功并生成 MySQL delivery：

```text
unread +1
```

接收客户端写盘并 SHA-256 校验成功，服务器收到：

```text
FILE_RECEIVED
```

后：

```text
MySQL delivered
Redis unread -1
```

Redis 仍然只是派生缓存。

如果 Redis unread 被清空：

```text
MySQL file_transfer_deliveries
```

仍然决定哪些文件需要重投。


## v8.4 BLOCK_FRIEND

好友屏蔽的可靠权限状态存 MySQL `friend_blocks`，不放 Redis。

原因：

```text
Redis = cache
MySQL = durable permission truth
```

如果 direct item 在被 block 以前已经进入 pending，Redis 聚合 unread 数字可能暂时仍包含该 item；MySQL delivery 查询会阻止真正投递。解除屏蔽并完成投递后，Redis unread 再按原机制递减。
