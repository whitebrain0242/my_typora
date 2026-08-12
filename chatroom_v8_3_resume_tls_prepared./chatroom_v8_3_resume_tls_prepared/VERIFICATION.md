# v8.3 Verification Report

## 当前环境

检测到：

```text
GNU C++ 14
CMake 3.31
mysqlclient development library
hiredis 1.2.0
sqlite3 3.46.1
OpenSSL 3.5.6
```

当前环境缺：

```text
protobuf-compiler (protoc)
libprotobuf-dev headers
```

因此最终官方 Protobuf CMake configure 会在：

```cmake
find_package(Protobuf REQUIRED)
```

处停止。

实际 CMake probe 得到：

```text
Could NOT find Protobuf
(missing: Protobuf_LIBRARIES Protobuf_INCLUDE_DIR)
```

v8.3 没有 custom codec fallback。

---

## 项目源码编译检查

由于缺官方 Protobuf headers，本环境临时创建了仅供编译器检查 generated-class API 形状的 stub。

该 stub：

```text
不进入最终 ZIP
不替代 libprotobuf
不用于目标机运行
```

使用该 stub 编译完整服务端：

```text
Main/Sub Reactor
TLS TcpConnection
ChatServer
FileTransferService
MYSQL_STMT MySqlDatabase
Redis
server_main
```

结果：

```text
PASS
-Wall -Wextra -Wpedantic
0 warnings
```

客户端不依赖 generated Protobuf class，直接以真实：

```text
OpenSSL
SQLite
```

完整编译：

```text
PASS
0 warnings
```

---

## TLS 实测

真实 OpenSSL + 临时开发 CA/certificate。

### TLS context

```text
CA verified
IP identity 127.0.0.1 verified
SSL_connect / SSL_accept
SSL_write / SSL_read
PASS
```

### TLS Reactor

真实：

```text
OpenSSL client
↓
TLS
↓
TcpServer
↓
SubReactor
↓
TcpConnection nonblocking SSL_accept
↓
SSL_read
↓
message callback
↓
SSL_write
↓
client
```

echo：

```text
PASS
```

---

## Resume 实测

### 服务端 upload checkpoint

测试：

```text
begin upload
append first chunk
simulate disconnect
read .resume.pb
read .part size
restore original recipient snapshot
continue from offset
append second chunk
full SHA
finalize
```

在本环境因为官方 Protobuf headers 缺失，resume sidecar round-trip 测试使用临时 API stub 的最小序列化，仅用于验证 FileTransferService 的 checkpoint/offset/recipient 逻辑。

结果：

```text
PASS
0 warnings
```

正式官方 Protobuf `FileUploadResumeState` CTest 需在安装 `protoc/libprotobuf-dev` 后运行。

### SQLite resume

真实 sqlite3 `:memory:`：

```text
pending_uploads save/list/remove
partial_downloads save/get/remove
message history
file history
stats
```

结果：

```text
PASS
```

---

## Reactor regression

TLS 修改后重新执行：

```text
master/sub Reactor smoke           PASS
round-robin distribution           PASS
cross-thread eventfd send          PASS
```

---

## Redis regression

hiredis 对测试 RESP server：

```text
presence
message unread
file unread
```

结果：

```text
PASS
```

---

## MySQL Prepared Statement contract

源码检查：

```text
mysql_stmt_prepare         present
mysql_stmt_bind_param      present
mysql_stmt_execute         present
mysql_stmt_bind_result     present

mysql_real_escape_string   0
escape(                    0
mysql_query(               0
mysql_real_query(          0
CLIENT_MULTI_STATEMENTS    0
std::to_string(            0
```

`tests/check_mysql_prepared.sh`：

```text
PASS
```

完整服务端也成功链接真实 mysqlclient development library。

但当前环境没有：

```text
mysqld / mariadbd
mysql CLI
```

因此没有执行真实 MySQL server 的端到端 CRUD/transaction integration。

---

## Sanitizers

使用：

```text
AddressSanitizer
UndefinedBehaviorSanitizer
Leak detection
```

实际执行：

```text
SQLite resume test         PASS
TLS Reactor echo           PASS
file resume storage logic  PASS
```

---

## 目标机必须再次执行

安装官方 Protobuf：

```bash
sudo apt install -y \
  protobuf-compiler \
  libprotobuf-dev
```

然后：

```bash
cmake -S . -B build
cmake --build build

ctest \
  --test-dir build \
  --output-on-failure
```

再用真实：

```text
MySQL
Redis
Alice client
Bob client
```

验收：

```text
TLS login/chat
在线/离线文件
上传中断后重启客户端继续
上传中断后重启服务端继续
下载中断后继续
群文件 resume
Prepared Statement CRUD
```
