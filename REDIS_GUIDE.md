# Redis Guide — v8.2

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
