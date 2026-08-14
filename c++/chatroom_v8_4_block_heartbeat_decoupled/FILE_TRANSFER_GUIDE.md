# File Transfer Guide — v8.4

v8.4 文件模块继续支持：

```text
私聊在线/离线文件
群组在线/离线文件
上传断点续传
下载断点续传
SHA-256 完整性确认
SQLite 本地状态
MySQL durable delivery
Redis 文件未读缓存
```

## 用户命令

```text
SEND_FILE <friend> <path>
SEND_GROUP_FILE <group> <path>
RESUME_UPLOADS
LOCAL_FILES [count]
PENDING
```

## 上传

```text
SEND_FILE
↓
客户端计算 size + SHA-256
↓
SQLite pending_uploads
↓
FILE_BEGIN...
↓
服务器检查权限/metadata
↓
tmp/<token>.part
tmp/<token>.resume.pb
↓
FILE_READY token offset
↓
客户端 seekg(offset)
↓
FILE_CHUNK...
```

断线时 `.part`、`.resume.pb` 和 SQLite task 都保留。

`resume.pb` 使用官方 `FileUploadResumeState` Protobuf。

## 下载

```text
server FILE_OFFER
↓
client 查 partial_downloads + .part
↓
FILE_RESUME_REQUEST id local_offset
↓
server seekg(local_offset)
↓
FILE_RESUME_START
FILE_DATA...
FILE_DONE
↓
client full SHA-256
↓
FILE_RECEIVED
↓
MySQL delivered
```

## 失败处理

可恢复的上传问题：

```text
FILE_PAUSED
```

服务端保留 checkpoint。

显式取消：

```text
FILE_ABORT
```

服务端删除 `.part + .resume.pb`。

下载中断：

```text
.part 保留
partial_downloads 保留
```

最终 SHA 错误：

```text
认为 partial 已损坏
删除 partial
下次从 0 开始
```

## 文件字节传输

仍保持 v8.2 的文本协议兼容方案：

```text
raw chunk: 3072 bytes
↓
Base64
↓
one FILE_DATA line
```

TLS 在其下层加密整个 TCP application stream，所以 Base64 内容、登录命令和所有聊天文本在网络上都位于 TLS records 内。


## v8.4 私聊文件屏蔽

`BLOCK_FRIEND` 同时作用于私聊文件。

检查点：

```text
FILE_BEGIN_PRIVATE
↓
DirectMessagePolicy

FILE_END
↓
DirectMessagePolicy re-check

FILE_OFFER delivery
↓
is_friend_blocked re-check
```

因此不能通过：

```text
先开始上传
再被 recipient 屏蔽
最后继续 FILE_END
```

绕过屏蔽策略。

群文件不受好友屏蔽影响。
