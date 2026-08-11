# chatroom v8.2 — 私聊/群组文件传输 + 官方 Protobuf

本版本直接在 **v8.1（主从 Reactor + 群组 + 离线消息 + 服务端 Redis + 客户端 SQLite）** 上继续演进。

本版两个核心目标：

1. 私聊、群聊都支持在线文件发送，以及给离线用户发送文件后在其上线时自动通知/接收。
2. 完全删除自定义 Protobuf wire 编解码器，改用官方 `protoc` 生成 C++ 类并链接官方 `libprotobuf`。

---

## 1. 总体架构

```text
Main thread
└── MainReactor
    └── Acceptor

SubReactor worker pool
├── client A TcpConnection
├── client B TcpConnection
└── ...

ChatServer business layer
├── MySQL
│   ├── messages
│   ├── group_messages
│   ├── file_transfers
│   └── file_transfer_deliveries
│
├── Redis
│   ├── presence
│   ├── private/group message unread
│   └── private/group file unread
│
└── FileTransferService
    ├── storage/tmp/*.part
    ├── storage/files/*
    └── 2 disk-read worker threads

chat_client
├── SQLite local messages
├── SQLite local file records
└── downloads/<username>/*
```

Reactor 网络核心没有因为文件传输再次重写。

---

## 2. 新增用户命令

### 私聊文件

```text
SEND_FILE <username> <path>
```

示例：

```text
SEND_FILE bob ./documents/report.pdf
SEND_FILE bob "/home/alice/My Report.pdf"
```

路径中有空格也可以，因为客户端把第一个参数当用户名，剩余整段作为路径。

### 群文件

```text
SEND_GROUP_FILE <group_name> <path>
```

示例：

```text
SEND_GROUP_FILE cpp_study ./demo.zip
```

只有当前群成员可以发送群文件。

### 查看本机文件记录

```text
LOCAL_FILES [count]
```

例如：

```text
LOCAL_FILES
LOCAL_FILES 50
```

该命令只查询客户端 SQLite，不访问服务器。

---

## 3. 私聊文件规则

私聊文件和私聊消息保持同样的好友约束：

```text
Alice -> Bob
```

要求：

- Bob 账号存在；
- Alice 与 Bob 已经是好友；
- Bob 是否在线不影响 Alice 上传。

如果 Bob 在线：

```text
Alice upload
→ server validates/stores
→ MySQL file delivery
→ Bob immediately receives
```

如果 Bob 离线：

```text
Alice upload
→ server validates/stores
→ MySQL pending delivery
→ Redis private_file unread +1
→ Bob login
→ automatic download
→ Bob local SHA-256 verified
→ FILE_RECEIVED acknowledgement
→ MySQL delivered
→ Redis private_file unread -1
```

---

## 4. 群文件规则

发送：

```text
SEND_GROUP_FILE cpp_study ./slides.pdf
```

服务器在文件提交成功时快照当前群成员列表：

```text
sender excluded
other member A -> delivery row
other member B -> delivery row
other member C -> delivery row
```

因此每个成员有独立的：

```text
delivered_at_unix_ms
```

在线成员立即接收；离线成员上线后接收。

如果某用户发送时属于群组、随后退出群组，已经生成的那条文件 delivery 仍然属于他，因为它代表“发送当时已面向该成员发送”。

群组后来被解散也不会删除已经生成的文件记录；`file_transfers.group_id` 外键使用 `ON DELETE SET NULL`，使历史待投递文件仍可完成。

---

## 5. 文件上传协议

用户不会直接输入以下命令，它们是客户端和服务端之间的内部协议。

客户端执行：

```text
SEND_FILE bob ./a.pdf
```

首先在本地：

```text
检查文件
→ 获取 size
→ 计算 SHA-256
→ 生成随机 transfer_token
```

然后只发送元数据：

```text
FILE_BEGIN_PRIVATE
```

服务器检查：

```text
登录状态
好友/群权限
文件大小
token
SHA-256 格式
```

接受后：

```text
FILE_READY <token>
```

客户端这时才真正开始发送：

```text
FILE_CHUNK <token> <offset> <base64>
FILE_CHUNK ...
FILE_END <token>
```

这样，如果权限检查失败，大文件不会先白白上传。

---

## 6. 为什么文件内容使用 Base64 分块

现有项目协议从 v1 到 v8.1 一直是：

```text
一行文本命令 + '\n'
```

本版没有为了文件功能把整个聊天协议推翻成新的二进制帧协议。

每块原始文件大小：

```text
3072 bytes
```

Base64 后约：

```text
4096 characters
```

仍然安全低于服务器单行输入保护上限。

所以本版：

```text
文件元数据/聊天结构化数据
→ official Protobuf

实际文件字节
→ Base64 chunk lines
```

这里的 Base64 是传输封装，不是自定义 Protobuf 编码。

---

## 7. 服务端文件校验

上传过程中：

```text
offset 必须连续
chunk <= 3072 bytes
累计大小不能超过声明大小
```

`FILE_END` 时再次检查：

```text
actual size == declared size
SHA-256(actual file) == declared SHA-256
```

失败：

```text
FILE_REJECT
```

并删除 `.part` 临时文件。

成功后：

```text
storage/tmp/<token>.part
↓
storage/files/<token>_<safe_filename>
↓
MySQL file_transfers
↓
MySQL file_transfer_deliveries
```

本版课程项目文件上限：

```text
20 MiB
```

---

## 8. 下载与真正的“已接收”

服务端不会因为：

```cpp
TcpConnection::send(...)
```

调用成功就认为文件已到达客户端。

流程是：

```text
FILE_OFFER
FILE_DATA ...
FILE_DONE
       ↓
client writes .part
       ↓
client checks exact byte count
       ↓
client computes SHA-256
       ↓
rename to final file
       ↓
SQLite cache
       ↓
FILE_RECEIVED <id> <sha256>
       ↓
server validates pending metadata
       ↓
MySQL mark delivered
       ↓
Redis unread -1
```

因此：

- 网络中断；
- 客户端写盘失败；
- SHA-256 不匹配；

都不会把 MySQL delivery 错误标成完成。

下一次登录或：

```text
PENDING
```

仍能重新下发整个文件。

---

## 9. 当前版本不做断点续传

本版实现的是：

```text
可靠整文件重投
```

而不是：

```text
resume from offset
```

断线后 `.part` 会被清理，下一次从 0 开始重新下载。

后续若要做断点续传，可以在现有：

```text
transfer_id
file_size
sha256
chunk offset
delivery state
```

基础上增加：

```text
FILE_RESUME <transfer_id> <offset>
```

无需再重构主从 Reactor。

---

## 10. 文件磁盘 I/O 与 Reactor

下载时，读取服务端磁盘文件不直接在 SubReactor 中循环执行。

新增：

```text
FileTransferService
└── default 2 worker threads
```

工作线程：

```text
read stored file
→ Base64
→ connection->send()
```

`TcpConnection::send()` 已经是线程安全接口，会把发送工作 `queueInLoop()` 回该连接所属 SubReactor，并通过 `eventfd` 唤醒。

这样较大的服务端文件读取不会直接卡住某个 Reactor 的 socket 事件处理。

上传端每次只写一个 3072-byte 小块到临时文件，保持实现简单。

---

## 11. 官方 Protobuf

v8.1 原来有：

```text
include/proto_codec.hpp
src/proto_codec.cpp
```

它们已经在 v8.2 **删除**。

现在 CMake：

```cmake
find_package(Protobuf REQUIRED)

add_library(chat_proto ...)
protobuf_generate(
    TARGET chat_proto
    LANGUAGE cpp
    ...
)

target_link_libraries(
    chat_proto
    PUBLIC protobuf::libprotobuf
)
```

构建时由官方 `protoc` 生成：

```text
friend_event.pb.h/.cc
chat_message.pb.h/.cc
group_message.pb.h/.cc
file_transfer.pb.h/.cc
```

业务代码直接调用：

```cpp
payload.SerializeToString(&bytes);
payload.ParseFromArray(data, size);
```

不再存在任何自己实现的 varint/tag/wire parser。

---

## 12. 旧 MySQL Protobuf BLOB 是否需要清空

不需要。

v8.2 保留了原来的 `.proto`：

```text
friend_event.proto
chat_message.proto
group_message.proto
```

以及原字段号和类型。

所以过去按 proto3 wire format 写入的 BLOB 可以由官方生成类继续解析。

测试中专门保留了一组 v7.x 固定 wire bytes，并交给官方生成类解析，用于验证迁移兼容路径。

---

## 13. 新的 FileTransfer Protobuf

新增：

```text
proto/file_transfer.proto
```

主要字段：

```text
transfer_token
scope
sender_username
recipient_username
group_id
group_name
file_name
file_size
sha256_hex
created_at_unix_ms
stored_relative_path
```

MySQL：

```text
file_transfers.metadata LONGBLOB
```

保存的是官方 Protobuf 序列化结果。

---

## 14. MySQL 升级

从 v8.1 升级：

```bash
sudo mysql chatroom < sql/005_create_file_transfers.sql
```

或者：

```bash
sudo mysql < MYSQL_UPGRADE_V8_1_TO_V8_2.sql
```

新增：

```text
file_transfers
file_transfer_deliveries
```

如果全新安装：

```bash
sudo mysql < MYSQL_RUN_THIS_V8_2.sql
```

---

## 15. Redis

新增未读 fields：

```text
private_file
group_file
```

例如：

```bash
redis-cli HGETALL chatroom:unread:bob
```

可能：

```text
private
2
group
1
private_file
3
group_file
4
```

注意：

```text
Redis = cache
MySQL delivery = durable truth
```

Redis 丢失未读计数并不会删除服务器保存的待投递文件。

---

## 16. SQLite

客户端自动新增：

```text
file_transfers
```

保存：

```text
server_transfer_id
scope
peer/group
sender
file_name
local_path
file_size
sha256
received_at
outgoing
```

收到文件只有完成本地 SHA-256 校验后才写入 SQLite。

发送文件在服务器返回：

```text
FILE_UPLOAD_OK
```

后写入本地记录。

---

## 17. 安装依赖

Ubuntu / Debian / WSL：

```bash
sudo apt update

sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    default-libmysqlclient-dev \
    libssl-dev \
    libhiredis-dev \
    libsqlite3-dev \
    protobuf-compiler \
    libprotobuf-dev \
    redis-server
```

关键新增的是：

```text
protobuf-compiler
libprotobuf-dev
```

因为 v8.2 不再内置任何 Protobuf 编码实现。

---

## 18. 编译

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

构建目录中会自动生成官方 `.pb.h/.pb.cc`。

---

## 19. 运行

服务端：

```bash
./build/chat_server \
    9000 \
    config/mysql.conf \
    config/redis.conf \
    4 \
    data/server_files
```

最后一个参数是服务器文件存储根目录。

客户端：

```bash
./build/chat_client \
    127.0.0.1 \
    9000 \
    data/alice.db \
    downloads
```

客户端最终文件默认：

```text
downloads/<logged_in_username>/<transfer_id>_<filename>
```

---

## 20. 推荐验收

### 在线私聊文件

Alice：

```text
LOGIN alice ...
SEND_FILE bob ./test.pdf
```

Bob 在线，应立即自动接收。

### 离线私聊文件

Bob：

```text
LOGOUT
```

Alice：

```text
SEND_FILE bob ./offline.zip
```

Bob 再登录，应自动看到 pending file 提示并接收。

### 在线群文件

```text
SEND_GROUP_FILE cpp_study ./demo.zip
```

所有在线其他成员立即接收。

### 离线群成员

一个群成员离线后发送群文件，再让其登录，应自动接收。

### 客户端本地记录

```text
LOCAL_FILES 50
LOCAL_STATS
```

### Redis

```bash
redis-cli HGETALL chatroom:unread:bob
```

### SQLite

```bash
sqlite3 data/bob.db
```

```sql
SELECT *
FROM file_transfers
ORDER BY server_transfer_id;
```

---

## 21. 本版边界

已完成：

- 私聊在线文件
- 私聊离线文件
- 群在线文件
- 群离线文件
- 登录自动接收
- `PENDING` 重试
- SHA-256 双端校验
- MySQL durable delivery
- Redis file unread
- SQLite local file records
- official Protobuf
- 删除自定义 wire codec

暂未做：

- 断点续传
- 文件删除/过期清理策略
- CDN/对象存储
- 多服务器共享文件存储
- 文件压缩
- 文件内容加密
