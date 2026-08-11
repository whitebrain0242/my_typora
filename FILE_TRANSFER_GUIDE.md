# v8.2 File Transfer Guide

## 用户命令

```text
SEND_FILE <friend> <path>
SEND_GROUP_FILE <group> <path>
LOCAL_FILES [count]
```

## 内部上传协议

```text
client:
FILE_BEGIN_PRIVATE token bob filename_b64 size sha256

server:
FILE_READY token

client:
FILE_CHUNK token 0 base64
FILE_CHUNK token 3072 base64
...
FILE_END token

server:
FILE_UPLOAD_OK token transfer_id
```

群文件使用：

```text
FILE_BEGIN_GROUP
```

客户端异常读文件时：

```text
FILE_ABORT token
```

## 内部下载协议

```text
server:
FILE_OFFER id scope sender group_b64_or_- filename_b64 size sha256
FILE_DATA id offset base64
...
FILE_DONE id

client:
write file
verify size
verify SHA-256
rename .part -> final
SQLite cache
FILE_RECEIVED id sha256

server:
MySQL delivered
Redis unread -1
FILE_ACK_OK id
```

## 服务端目录

```text
data/server_files/
├── tmp/
│   └── <token>.part
└── files/
    └── <token>_<filename>
```

## 客户端目录

```text
downloads/
└── alice/
    ├── 12_report.pdf
    └── 18_demo.zip
```

## 私聊权限

```text
sender/recipient 必须是好友
```

不要求 recipient 在线。

## 群权限

```text
sender 必须是当前群成员
```

接收人列表在成功提交文件时对其他群成员做快照。

## 数据库

```text
file_transfers
file_transfer_deliveries
```

`file_transfers.metadata` 是官方 Protobuf `FileTransferMetadata`。

## 文件上限

```text
20 MiB
```

## chunk

```text
raw: 3072 bytes
transport: Base64
```

## 成功定义

服务器把文件写进 socket buffer 并不等于 delivered。

只有接收客户端：

```text
本地写盘成功
+
本地 SHA-256 成功
+
FILE_RECEIVED
```

服务端才更新 MySQL delivery。

## 离线

MySQL pending delivery 不依赖 Redis。

Redis 只维护：

```text
private_file
group_file
```

未读缓存。

## 重试

如果连接断开：

```text
delivered_at_unix_ms remains NULL
```

下次：

```text
LOGIN
或 PENDING
```

整文件重新发送。

## 当前不是断点续传

本版没有保存客户端 partial offset。

后续可加入：

```text
FILE_RESUME transfer_id offset
```


## 客户端本地接收失败

如果最终：

```text
size
SHA-256
rename
```

任一失败，客户端发送：

```text
FILE_RECEIVE_FAILED <transfer_id>
```

服务端不标记 delivered，并清除本连接的 in-progress 状态。

用户可以直接：

```text
PENDING
```

重新尝试整文件下载，不一定要重新登录。
