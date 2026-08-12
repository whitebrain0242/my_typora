# v8.2 → v8.3 Migration

## MySQL schema

v8.3 没有新增 MySQL table。

如果 v8.2 的：

```text
sql/005_create_file_transfers.sql
```

已经执行，则不需要新的 MySQL migration。

变化是 C++ 数据访问方式：

```text
string SQL
→ MYSQL_STMT
```

## SQLite

客户端启动时自动：

```text
CREATE TABLE IF NOT EXISTS pending_uploads
CREATE TABLE IF NOT EXISTS partial_downloads
```

不需要手工 migration。

## TLS

新增必需配置：

```text
config/tls_server.conf
config/tls_client.conf
```

开发环境：

```bash
./scripts/generate_dev_tls_cert.sh

cp config/tls_server.conf.example \
   config/tls_server.conf

cp config/tls_client.conf.example \
   config/tls_client.conf
```

## 新服务端命令行

```bash
./build/chat_server \
  9000 \
  config/mysql.conf \
  config/redis.conf \
  config/tls_server.conf \
  4 \
  data/server_files
```

## 新客户端命令行

```bash
./build/chat_client \
  127.0.0.1 \
  9000 \
  data/alice.db \
  downloads \
  config/tls_client.conf
```

## v8.2 文件任务

已经完成并保存到 MySQL 的文件不受影响。

v8.2 中“尚未完成上传”的 `.part` 没有 resume sidecar，因此不能安全解释为 v8.3 checkpoint。

v8.3 的 `begin_or_resume_upload` 遇到 orphaned `.part`/`.resume.pb` 会清理并重新创建一致 checkpoint。
