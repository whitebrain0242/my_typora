# v8.2 Verification Report

## 1. 当前执行环境限制

当前执行环境已经有：

```text
C++ compiler
CMake
MySQL client development library
hiredis
sqlite3
OpenSSL
```

但没有安装：

```text
protobuf-compiler (protoc)
libprotobuf-dev headers
```

因此直接执行最终：

```bash
cmake -S . -B build
```

会在：

```cmake
find_package(Protobuf REQUIRED)
```

处停止，并报告缺少：

```text
Protobuf_LIBRARIES
Protobuf_INCLUDE_DIR
```

这说明 v8.2 确实依赖外部官方 Protobuf，而不再偷偷回退到项目自己的 codec。

本环境无法完成“官方 protoc 生成 + 官方 libprotobuf 编译”的最终一步，所以没有把该项伪装成 PASS。

在安装：

```bash
sudo apt install protobuf-compiler libprotobuf-dev
```

的目标机上需要执行完整 CMake/CTest。

---

## 2. 官方 Protobuf 源码迁移检查

源码树确认：

```text
include/proto_codec.hpp    已删除
src/proto_codec.cpp        已删除
```

不存在：

```text
serialize_chat_message
parse_chat_message
serialize_group_message
parse_group_message
hand-written varint/tag parser
```

最终 CMake 使用：

```text
find_package(Protobuf REQUIRED)
protobuf_generate
protobuf::libprotobuf
```

数据库层使用：

```text
SerializeToString
ParseFromArray
```

业务层使用官方生成类 setters/getters。

---

## 3. 生成类 API 形状编译检查

为了在缺少 `protoc` header 的执行环境中继续排查项目自身的 C++ 错误，
测试目录外临时创建了最小 generated-message API stub。

这些 stub：

```text
不会进入最终 ZIP
不是 Protobuf 实现
不用于最终运行
```

仅用于让编译器检查：

```text
ChatServer
MySqlDatabase
FileTransferService
server_main
全部 Reactor source
```

是否正确使用预期的 generated C++ API。

结果：

```text
full server source/link shape: PASS
-Wall -Wextra -Wpedantic: 0 warnings
```

该检查不能替代真实 `protoc + libprotobuf` 构建，最终仍需在安装官方开发包的机器运行 CMake。

---

## 4. 客户端真实编译

`chat_client` 不直接链接 Protobuf，因此本环境可以完整编译：

```text
client build: PASS
-Wall -Wextra -Wpedantic: 0 warnings
```

包含：

```text
SEND_FILE
SEND_GROUP_FILE
Base64 chunks
SHA-256
automatic downloads
FILE_RECEIVED
SQLite file cache
LOCAL_FILES
```

---

## 5. 实际执行的测试

### Password

```text
PASS
```

### Buffer

```text
PASS
```

### Master/Sub Reactor smoke

```text
PASS
```

真实 TCP client/server 测试。

### Reactor round-robin distribution

```text
PASS
```

验证多个连接分配到不同 SubReactor worker。

### Cross-thread eventfd send

```text
PASS
```

验证非连接所属线程调用 `TcpConnection::send()` 后可通过 EventLoop/eventfd 回到目标 Reactor。

### File utilities

```text
PASS
```

实际测试：

```text
Base64 round-trip
SHA-256("abc")
random transfer token
filename sanitization
```

### File transfer storage

```text
PASS
```

实际测试：

```text
begin temp upload
append multiple chunks
offset tracking
SHA-256
finalize upload
safe filename
final stored file
```

### SQLite

```text
PASS
```

真实 sqlite3 `:memory:` 数据库，测试：

```text
private message cache
group message cache
file transfer cache
file history
stats
```

### Redis

```text
PASS
```

真正 hiredis client 连接测试用 RESP server，验证：

```text
presence
private/group unread
private_file/group_file unread
```

---

## 6. 历史 Protobuf 兼容测试

`tests/test_proto.cpp` 中保留 v7.x 的固定 FriendEvent wire bytes。

正式官方 Protobuf 环境构建后，该 test 会：

```text
official generated FriendEventPayload
→ ParseFromString(historical bytes)
```

并检查：

```text
alice
bob
timestamp
event type
```

用于防止 v8.2 的官方库迁移破坏历史 MySQL payload。

该 test 因当前机器缺 `protoc/libprotobuf-dev` 尚未在官方运行时下执行。

---

## 7. 尚未在本环境端到端执行

当前环境没有运行中的：

```text
MySQL server
Redis server
```

也缺官方 Protobuf 开发包。

因此还需要在你的 Linux/WSL 环境执行最终端到端验收：

```text
MySQL 005 migration
Redis
official protoc build
Alice/Bob client
online private file
offline private file
online group file
offline group file
client SHA verification
SQLite LOCAL_FILES
Redis file unread
server restart + pending file delivery
```

---

## 8. 推荐最终验收命令

```bash
sudo apt install -y \
    default-libmysqlclient-dev \
    libssl-dev \
    libhiredis-dev \
    libsqlite3-dev \
    protobuf-compiler \
    libprotobuf-dev \
    redis-server

cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
