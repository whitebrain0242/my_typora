# SQLite Guide — v8.2

原有：

```text
private_messages
group_messages
```

新增：

```text
file_transfers
```

字段包括：

```text
account_username
server_transfer_id
scope
peer_username
group_name
sender_username
file_name
local_path
file_size
sha256_hex
received_at_unix_ms
outgoing
```

## 收到文件

顺序：

```text
download .part
→ exact size
→ SHA-256
→ rename final
→ cache_file_transfer()
→ FILE_RECEIVED
```

因此 SQLite 中的 received file record 对应的是已经完成本地校验的文件。

## 发送文件

收到服务器：

```text
FILE_UPLOAD_OK token transfer_id
```

后记录发送文件。

## 查询

```text
LOCAL_FILES [count]
LOCAL_STATS
LOCAL_DB
```

例如：

```text
LOCAL_FILES 20
```

输出：

```text
#F12 [PRIVATE] received report.pdf ...
#F15 [GROUP] sent demo.zip ...
```
