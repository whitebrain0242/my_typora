# v8.2 Architecture

## 网络

```text
MainReactor
└── accept

SubReactor pool
├── connection A
├── connection B
└── connection C
```

文件功能没有改变 socket ownership。

## 文件上传

```text
chat_client
  │
  │ FILE_BEGIN
  ▼
SubReactor
  │ permission check
  ▼
server tmp file
  │
  │ FILE_CHUNK
  ▼
append small chunks
  │
  │ FILE_END
  ▼
size + SHA-256
  │
  ▼
final storage
  │
  ├── MySQL metadata
  ├── MySQL delivery rows
  └── Redis unread
```

## 文件下载

```text
MySQL pending
    │
    ▼
FileTransferService queue
    │
    ▼
file worker thread
    │ read file + Base64
    │
    ▼
TcpConnection::send
    │
    ▼
queueInLoop / eventfd
    │
    ▼
target SubReactor
    │
    ▼
client socket
```

## 收件确认

```text
client FILE_DONE
  ↓
local size check
  ↓
local SHA-256
  ↓
final rename
  ↓
SQLite
  ↓
FILE_RECEIVED
  ↓
server MySQL mark delivered
  ↓
Redis unread decrement
```

## Official Protobuf

```text
.proto
  ↓
official protoc
  ↓
generated *.pb.h / *.pb.cc
  ↓
libprotobuf
  ↓
SerializeToString / ParseFromArray
```

自定义 wire codec 已移除。

## 数据分工

```text
MySQL:
durable metadata + delivery state

Redis:
presence + derived unread counters

Server filesystem:
actual file bytes

SQLite:
client local file/message index

Client filesystem:
downloaded actual file bytes
```
