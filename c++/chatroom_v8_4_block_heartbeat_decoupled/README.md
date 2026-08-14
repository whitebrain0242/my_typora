# chatroom v8.4 — 好友消息屏蔽 + TCP/TLS 心跳 + 客户端/服务端解耦

v8.4 基于 v8.3，保留：

```text
Main/Sub Reactor
TLS 1.2/1.3
官方 Protobuf
MySQL Prepared Statement
Redis
SQLite
好友
群组
私聊/群聊
离线消息
在线/离线文件
上传/下载断点续传
SHA-256
```

新增：

```text
1. BLOCK_FRIEND / UNBLOCK_FRIEND / BLOCKED_FRIENDS
2. 私聊文字 + 私聊文件屏蔽策略
3. 服务端 PING/PONG 心跳
4. 客户端 heartbeat timeout
5. Linux TCP SO_KEEPALIVE/TCP_KEEP*
6. 服务端 Registry / Policy / Heartbeat / Router 解耦
7. 客户端 App / Transport / Heartbeat / File / LocalCommand / Cache 解耦
```

---

## 1. 依赖

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

---

## 2. v8.3 → v8.4 MySQL 升级

如果已经有 v8.3 数据库：

```bash
sudo mysql chatroom \
  < sql/006_create_friend_blocks.sql
```

或：

```bash
sudo mysql \
  < MYSQL_UPGRADE_V8_3_TO_V8_4.sql
```

全新安装：

```bash
sudo mysql \
  < MYSQL_RUN_THIS_V8_4.sql
```

新增表：

```text
friend_blocks
```

---

## 3. TLS 开发证书

与 v8.3 一样：

```bash
./scripts/generate_dev_tls_cert.sh

cp config/tls_server.conf.example \
   config/tls_server.conf

cp config/tls_client.conf.example \
   config/tls_client.conf
```

不要提交：

```text
ca.key
server.key
```

---

## 4. 编译

```bash
cmake -S . -B build
cmake --build build

ctest \
  --test-dir build \
  --output-on-failure
```

---

## 5. 服务端

```bash
./build/chat_server \
  9000 \
  config/mysql.conf \
  config/redis.conf \
  config/tls_server.conf \
  4 \
  data/server_files
```

默认 heartbeat：

```text
PING interval = 20s
server timeout = 60s
```

同时 socket 设置：

```text
SO_KEEPALIVE
TCP_KEEPIDLE=60
TCP_KEEPINTVL=15
TCP_KEEPCNT=3
```

---

## 6. 客户端

```bash
./build/chat_client \
  127.0.0.1 \
  9000 \
  data/alice.db \
  downloads \
  config/tls_client.conf
```

客户端：

```text
自动响应 PING
自动发送 PONG
75s 无任何 server activity → timeout
```

PING/PONG 不显示给用户，也不会进入 SQLite 聊天记录。

---

## 7. 好友消息屏蔽

Alice：

```text
BLOCK_FRIEND bob
```

查看：

```text
BLOCKED_FRIENDS
```

解除：

```text
UNBLOCK_FRIEND bob
```

屏蔽：

```text
MSG 私聊文字
SEND_FILE 私聊文件
离线私聊文字投递
离线私聊文件投递
```

不影响：

```text
好友关系
公共 SAY
群聊
群文件
旧聊天历史
好友在线状态
```

Bob 被 Alice 屏蔽后：

```text
MSG alice hello
```

服务器会拒绝新 direct delivery，不创建新的私聊 delivery。

---

## 8. 已经存在的离线消息怎么办

设计选择：

```text
block 前已经存入 MySQL 的 pending direct message/file
不会删除
不会伪装成 delivered
block active 时不投递
```

Alice：

```text
UNBLOCK_FRIEND bob
PENDING
```

之后可以继续收到这些旧 pending 数据。

---

## 9. 文件断点续传

原 v8.3 行为保持：

```text
client SQLite pending_uploads
server .part + .resume.pb
client partial_downloads + .part
```

并增加：

```text
private file begin
private file end
private file delivery
```

三个阶段的 block 检查，避免上传过程中用户突然被屏蔽后仍然完成私聊文件投递。

---

## 10. MySQL

v8.4 新增的 `friend_blocks` CRUD 同样全部使用：

```text
MYSQL_STMT
MYSQL_BIND
?
```

没有退回：

```text
mysql_query
mysql_real_query
mysql_real_escape_string
std::to_string SQL 拼接
```

---

## 11. 客户端架构

完整文档：

```text
CLIENT_ARCHITECTURE_V8_4.md
```

入口现在：

```text
src/client.cpp
```

只负责 bootstrap。

核心模块：

```text
ClientApp
TlsClientTransport
ClientHeartbeat
ClientState
client_file_transfer
client_local_commands
client_message_cache
SqliteClient
```

---

## 12. 服务端架构

完整文档：

```text
SERVER_ARCHITECTURE_V8_4.md
```

核心模块：

```text
Main/Sub Reactor
TcpConnection/TLS
ServerCommandRouter
ChatServer
OnlineUserRegistry
DirectMessagePolicy
HeartbeatManager
FileTransferService
MySqlDatabase
RedisClient
```

---

## 13. 推荐阅读顺序

```text
CHANGE_MAP_V8_3_TO_V8_4.md
↓
DECOUPLING_GUIDE_V8_4.md
↓
CLIENT_ARCHITECTURE_V8_4.md
↓
SERVER_ARCHITECTURE_V8_4.md
↓
FRIEND_BLOCKING_GUIDE.md
↓
HEARTBEAT_GUIDE.md
↓
SOURCE_CHANGES_V8_3_TO_V8_4.patch
```
