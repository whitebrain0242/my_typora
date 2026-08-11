# MySQL Setup — v8.2

## v8.1 → v8.2

只需要：

```bash
sudo mysql chatroom < sql/005_create_file_transfers.sql
```

新增：

```text
file_transfers
file_transfer_deliveries
```

也可以：

```bash
sudo mysql < MYSQL_UPGRADE_V8_1_TO_V8_2.sql
```

## 全新安装

先修改：

```text
MYSQL_RUN_THIS_V8_2.sql
config/mysql.conf.example
```

中的密码，然后：

```bash
sudo mysql < MYSQL_RUN_THIS_V8_2.sql
```

## file_transfers

保存：

```text
transfer_token
scope
sender
private recipient / group id
filename
size
SHA-256
server relative path
official protobuf metadata BLOB
```

实际文件 bytes 不放 MySQL BLOB，而是保存在服务器文件目录。

## file_transfer_deliveries

一个文件可以对应多个 recipient：

```text
transfer_id
recipient_username
delivered_at_unix_ms
```

群文件因此可以逐成员追踪投递。

## delivered 的定义

只有收到客户端：

```text
FILE_RECEIVED transfer_id sha256
```

并验证它与 MySQL metadata 匹配后，才写 delivered timestamp。
