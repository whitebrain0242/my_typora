# chatroom v8.3 — TLS + 断点续传 + MySQL Prepared Statements

v8.3 在 v8.2 基础上加入：

```text
1. 上传/下载双向断点续传
2. TLS 通信加密
3. MySQL MYSQL_STMT Prepared Statement
```

并继续保留：

```text
Main/Sub Reactor
好友
群组
私聊/群聊
离线消息
在线/离线文件
Redis
SQLite
official Protobuf
```

## 依赖

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

## TLS 开发证书

```bash
./scripts/generate_dev_tls_cert.sh

cp config/tls_server.conf.example \
   config/tls_server.conf

cp config/tls_client.conf.example \
   config/tls_client.conf
```

生成的私钥不要提交/分享。

## MySQL

全新数据库继续使用 v8.2 的完整安装脚本：

```bash
sudo mysql < MYSQL_RUN_THIS_V8_2.sql
```

如果已经是 v8.2 数据库：

```text
不需要新的 MySQL schema migration
```

v8.3 修改的是访问实现：

```text
SQL concatenation
→ MYSQL_STMT prepared statements
```

## 编译

```bash
cmake -S . -B build
cmake --build build

ctest \
  --test-dir build \
  --output-on-failure
```

## 服务端

```bash
./build/chat_server \
  9000 \
  config/mysql.conf \
  config/redis.conf \
  config/tls_server.conf \
  4 \
  data/server_files
```

参数：

```text
1 port
2 MySQL config
3 Redis config
4 TLS server config
5 SubReactor count
6 server file storage
```

## 客户端

```bash
./build/chat_client \
  127.0.0.1 \
  9000 \
  data/alice.db \
  downloads \
  config/tls_client.conf
```

连接成功会打印类似：

```text
[tls] version: TLSv1.3
[tls] cipher: ...
[tls] verified peer identity: 127.0.0.1
```

## 文件命令

```text
SEND_FILE <username> <path>
SEND_GROUP_FILE <group_name> <path>
RESUME_UPLOADS
LOCAL_FILES [count]
PENDING
```

### 上传断线

假设已上传 8 MiB：

```text
client disconnect
```

保留：

```text
SQLite pending_uploads
server tmp/<token>.part
server tmp/<token>.resume.pb
```

重新登录：

```text
LOGIN
↓
automatic pending upload reload
↓
FILE_BEGIN
↓
FILE_READY token 8388608
↓
seek local file to 8388608
↓
continue
```

### 下载断线

客户端保留：

```text
<id>_<filename>.part
SQLite partial_downloads
```

下一次服务器 `FILE_OFFER`：

```text
client file_size(.part)
↓
FILE_RESUME_REQUEST id offset
↓
server seekg(offset)
↓
continue FILE_DATA
```

## TLS

完整说明：

```text
TLS_GUIDE.md
```

## 断点续传

完整说明：

```text
RESUME_TRANSFER_GUIDE.md
```

## Prepared Statements

完整说明：

```text
MYSQL_PREPARED_GUIDE.md
```

## 新增代码如何看

优先阅读：

```text
V8_3_ADDED_CODE_GUIDE.md
```

想看逐行 diff：

```text
SOURCE_CHANGES_V8_2_TO_V8_3.patch
```

## 测试

新增：

```text
tls_context_verified
tls_reactor_echo
file_transfer_resume_storage
sqlite_local_cache_and_resume
mysql_prepared_statement_contract
```

原 Reactor 测试也继续保留。
