# v8.4 Verification Report

## 1. 当前环境

本执行环境检测到：

```text
GNU C++ 14
CMake 3.31
mysqlclient development library
hiredis 1.2.0
sqlite3 3.46.1
OpenSSL 3.5.6
```

当前环境仍然缺少：

```text
protobuf-compiler / protoc
libprotobuf-dev headers + library
```

并且没有可用的：

```text
mysqld / mariadbd
mysql CLI
```

因此下面严格区分：

```text
真实编译/真实运行
临时 generated-Protobuf API stub 的编译形状检查
静态 SQL/源码合约检查
尚未能在本环境执行的真实 MySQL/official-Protobuf E2E
```

---

# 2. 正式 CMake configure

实际执行：

```bash
cmake -S . -B <probe> \
  -DCMAKE_BUILD_TYPE=Release
```

探测成功：

```text
mysqlclient
hiredis
sqlite3
OpenSSL
```

随后按设计停止在：

```cmake
find_package(Protobuf REQUIRED)
```

错误：

```text
Could NOT find Protobuf
(missing: Protobuf_LIBRARIES Protobuf_INCLUDE_DIR)
```

项目没有 custom-codec fallback。

所以不能声称本环境完成了 official-Protobuf 全量 CMake build。

---

# 3. 客户端解耦编译

真实使用：

```text
OpenSSL
sqlite3
Linux sockets
```

编译新的：

```text
src/client.cpp
src/client/client_app.cpp
src/client/client_common.cpp
src/client/client_file_transfer.cpp
src/client/client_heartbeat.cpp
src/client/client_local_commands.cpp
src/client/client_message_cache.cpp
src/client/tls_client_transport.cpp
src/protocol.cpp
src/config.cpp
src/file_utils.cpp
src/sqlite_client.cpp
src/minimuduo/net/SocketOptions.cpp
src/minimuduo/net/TlsContext.cpp
```

结果：

```text
PASS
-Wall -Wextra -Wpedantic
0 warnings
```

结构审计：

```text
src/client.cpp lines      = 49
src/client.cpp SSL_read   = 0
src/client.cpp SSL_write  = 0
src/client.cpp sqlite3_   = 0
```

说明入口已经真正变成 bootstrap，不是只在文档中“逻辑分层”。

---

# 4. 服务端源代码/链接形状

由于当前环境缺 official generated `.pb.h`，使用一个位于项目目录外部的临时 generated-class API stub，只为了让编译器验证 C++ 调用形状。

stub：

```text
不进入最终 ZIP
不替代 official Protobuf
不用于目标机运行
```

真实链接：

```text
mysqlclient
hiredis
OpenSSL
pthread
```

覆盖：

```text
Main/Sub Reactor
TLS TcpConnection
SocketOptions
ServerCommandRouter
OnlineUserRegistry
DirectMessagePolicy
HeartbeatManager
FileTransferService
MySqlDatabase
RedisClient
ChatServer
server_main
```

最终结果：

```text
PASS
-Wall -Wextra -Wpedantic
0 warnings
```

结构审计：

```text
chat_server.cpp online_mutex_ = 0
chat_server.cpp command.name == giant dispatcher = 0
```

---

# 5. 新功能：服务端心跳

`tests/test_heartbeat.cpp` 使用真实：

```text
TcpServer
SubReactor
TcpConnection
HeartbeatManager
localhost TCP client
```

测试参数缩短为：

```text
PING 80ms
timeout 280ms
scan 20ms
```

客户端：

```text
回复前两个 PING
↓
停止 PONG
```

服务端：

```text
检测 timeout
↓
forceClose
```

结果：

```text
compile PASS
0 warnings
run PASS
```

---

# 6. 新功能：客户端自动 PONG

`tests/test_client_heartbeat.cpp` 使用真实 OpenSSL：

```text
development CA/server certificate
TLS server
TlsClientTransport
ClientHeartbeat
```

测试：

```text
Server: PING 4242
↓ TLS
ClientHeartbeat
↓
Client: PONG 4242
↓ TLS
Server verifies exact PONG
```

结果：

```text
compile PASS
0 warnings
run PASS
```

---

# 7. Heartbeat Sanitizer

新心跳 integration 使用：

```text
AddressSanitizer
UndefinedBehaviorSanitizer
Leak detection
```

实际执行：

```text
PASS
```

本运行环境需要 `LD_PRELOAD=libasan.so` 解决 ASan runtime 加载顺序问题；加载正确后没有 sanitizer 报错。

---

# 8. Reactor 回归

加入 SocketOptions/Heartbeat 后重新执行：

```text
master/sub Reactor smoke       PASS
round-robin distribution       PASS
cross-thread eventfd/send      PASS
TLS Reactor echo               PASS
```

所有重新编译目标：

```text
-Wall -Wextra -Wpedantic
0 warnings
```

---

# 9. SQLite / Redis / 文件续传回归

真实 SQLite：

```text
message cache
group cache
file cache
pending_uploads
partial_downloads
```

结果：

```text
PASS
```

Redis fake RESP server：

```text
presence/unread behavior regression
```

结果：

```text
PASS
```

文件续传 storage：

```text
.part
resume sidecar API
offset restore
recipient snapshot
SHA-256 finalize
```

结果：

```text
PASS
```

这一项因为本环境缺 official Protobuf headers，resume sidecar 的 generated-message API 使用项目目录外临时 stub 进行测试；因此它验证的是 FileTransferService 的 checkpoint/offset/文件逻辑，不等同于 official libprotobuf runtime 测试。

---

# 10. 好友屏蔽持久化合约

新增：

```text
tests/check_friend_block_contract.sh
```

实际检查：

```text
friend_blocks schema exists
directional primary key fields exist
is_friend_blocked exists
add_friend_block exists
remove_friend_block exists
list_blocked_friends exists
prepared '?' patterns exist
offline private-message query contains friend_blocks filter
offline private-file query contains friend_blocks filter
```

结果：

```text
friend-block persistence contract passed
```

---

# 11. MySQL Prepared Statement 回归

原 v8.3 合约继续执行：

```text
tests/check_mysql_prepared.sh
```

结果：

```text
MySQL prepared-statement source contract passed
```

最终源码审计：

```text
mysql_real_escape_string = 0
mysql_query(              = 0
mysql_real_query(         = 0
std::to_string( in mysql_database.cpp = 0
```

v8.4 新增的 `friend_blocks` CRUD 也全部通过现有 `MYSQL_STMT` helper。

---

# 12. 普通基础测试

重新运行：

```text
Buffer          PASS
Password/PBKDF2 PASS
file_utils      PASS
```

编译：

```text
0 warnings
```

---

# 13. Protobuf 自定义 codec 审计

`include/` + `src/` 扫描：

```text
proto_codec
write_varint
read_varint
skip_field
```

结果：

```text
0 hits
```

v8.4 没有重新引入手写 Protobuf wire codec。

---

# 14. 项目清洁度审计

项目树中：

```text
TLS .key/.crt/.csr       0
underscore temp artifacts 0
custom protobuf stub      0
test binary               0
```

开发证书必须由用户自己执行：

```bash
./scripts/generate_dev_tls_cert.sh
```

生成。

---

# 15. 当前不能声称通过的项目

## Official Protobuf full build

当前环境缺：

```text
protoc
libprotobuf-dev
```

所以目标机必须重新：

```bash
sudo apt install -y \
  protobuf-compiler \
  libprotobuf-dev

cmake -S . -B build
cmake --build build

ctest \
  --test-dir build \
  --output-on-failure
```

## Real MySQL E2E

当前环境无 MySQL server daemon，因此没有实际执行：

```text
CREATE friend_blocks
BLOCK_FRIEND
restart server
BLOCKED_FRIENDS
blocked offline delivery
UNBLOCK_FRIEND
PENDING
```

 against a live MySQL server.

代码已：

```text
编译/链接 mysqlclient
Prepared Statement contract PASS
SQL migration static contract PASS
```

但不能把它冒充为 live-MySQL integration PASS。

---

# 16. 建议你本机最终验收

安装依赖并执行完整 CTest 后，再使用 Alice/Bob 两客户端测试：

```text
Alice/Bob are friends
Alice BLOCK_FRIEND bob
Bob MSG alice ...
Bob SEND_FILE alice ...
→ both rejected

Alice BLOCKED_FRIENDS
→ bob listed

Alice UNBLOCK_FRIEND bob
Bob MSG alice ...
→ allowed
```

离线场景：

```text
Bob sends while Alice offline
Alice blocks Bob before PENDING
→ old pending stays stored but is not delivered

Alice unblocks
PENDING
→ old pending can deliver
```

心跳：

```text
normal client
→ invisible PING/PONG, connection stays alive

kill -STOP / network drop / test client stops PONG and sends no data
→ server timeout closes stale connection
```

最后重新测试：

```text
private/group chat
history
Redis
SQLite
TLS
online/offline file
resume upload/download
```

确认 v8.3 既有行为全部保持。
