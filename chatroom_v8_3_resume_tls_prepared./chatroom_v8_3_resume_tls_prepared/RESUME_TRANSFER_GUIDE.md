# Resumable File Transfer Guide — v8.3

## 上传

客户端 SQLite：

```text
pending_uploads
```

服务端：

```text
tmp/<token>.part
tmp/<token>.resume.pb
```

协议：

```text
FILE_BEGIN_PRIVATE/GROUP
↓
FILE_READY token server_offset
↓
FILE_CHUNK token server_offset ...
↓
...
FILE_END token
```

网络断开：

```text
client pending_uploads 保留
server .part/.resume.pb 保留
```

下一次登录：

```text
自动读取 pending_uploads
↓
重发 FILE_BEGIN
↓
服务器返回 file_size(.part)
↓
客户端 seekg(offset)
↓
继续上传
```

手工触发：

```text
RESUME_UPLOADS
```

## 下载

服务器：

```text
FILE_OFFER
```

客户端：

```text
partial_downloads
+
<id>_<filename>.part
```

客户端：

```text
offset = filesystem::file_size(.part)
FILE_RESUME_REQUEST id offset
```

服务器：

```text
seekg(offset)
FILE_RESUME_START id offset
FILE_DATA ...
FILE_DONE
```

完整 SHA 成功后：

```text
FILE_RECEIVED
```

才修改 MySQL delivery。

## 服务器重启

上传：

```text
resume.pb + part
```

都在磁盘，因此 ChatServer 内存状态丢失不影响 checkpoint。

下载：

```text
MySQL pending delivery
+
server final file
+
client partial file
+
client SQLite partial metadata
```

都可恢复。

## 安全边界

resume token 不是单独的授权凭证。

服务器恢复上传时仍检查：

```text
当前登录 username
私聊 friendship
群组 membership
metadata identity
token
size
SHA-256
```
