# v8.1 → v8.2 Migration

## 目标 1：文件传输

新增真实文件链路：

```text
chat_client local file
→ upload handshake
→ server temp storage
→ SHA-256
→ final storage
→ MySQL metadata/delivery
→ Redis unread
→ online/offline recipient
→ automatic download
→ client SHA-256
→ SQLite
→ FILE_RECEIVED
→ MySQL delivered
```

## 目标 2：官方 Protobuf

删除：

```text
include/proto_codec.hpp
src/proto_codec.cpp
```

添加：

```text
find_package(Protobuf REQUIRED)
protobuf_generate(...)
protobuf::libprotobuf
```

数据库读写全部改成官方生成对象 API。

## 不变部分

没有重构：

```text
EventLoop
Channel
Poller
Acceptor
TcpConnection
TcpServer
EventLoopThreadPool
```

已有：

```text
账号
好友
群组
公共消息
私聊
离线消息
Redis Presence
SQLite 消息缓存
```

均继续保留。

## 新 MySQL migration

```text
sql/005_create_file_transfers.sql
```

v8.1 数据库直接升级即可。

## 客户端新增第四个参数

v8.1：

```bash
chat_client ip port sqlite_db
```

v8.2：

```bash
chat_client ip port sqlite_db download_root
```

第四个参数可省略，默认：

```text
downloads
```

## 服务端新增第六个参数

```bash
chat_server port mysql.conf redis.conf workers file_storage_root
```

默认 file storage：

```text
data/server_files
```
