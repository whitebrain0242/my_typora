# SQLite Guide — v8.4

客户端 SQLite 当前包含：

```text
private_messages
group_messages
file_transfers
pending_uploads
partial_downloads
```

## pending_uploads

保存“尚未收到 FILE_UPLOAD_OK”的发送任务。

因此客户端重启后：

```text
LOGIN
↓
list_pending_uploads
↓
FILE_BEGIN
↓
服务器返回 checkpoint offset
↓
继续上传
```

## partial_downloads

保存下载文件的 identity：

```text
transfer id
sender/group
filename
temp path
expected size
expected SHA
```

真实 offset 不写 SQLite，而读取：

```cpp
filesystem::file_size(temp_path)
```

## 完成文件

仍写：

```text
file_transfers
```

只有完整 SHA-256 校验成功后才作为 received file 记录。

## SQLite migration

`SqliteClient::open()` 使用：

```sql
CREATE TABLE IF NOT EXISTS
```

自动补建新表。

参考：

```text
sql/client_sqlite_schema.sql
```
