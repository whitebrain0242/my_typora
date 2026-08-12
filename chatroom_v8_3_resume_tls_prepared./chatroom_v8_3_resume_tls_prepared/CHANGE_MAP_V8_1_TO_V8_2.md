# v8.1 → v8.2 精确文件变更清单

对比基线：`chatroom_v8_1_redis_sqlite.zip`。

最终下载包：**修改 26 个文件，新增 15 个文件，删除 2 个文件**。

## 修改文件

```text
ARCHITECTURE.md
BUILD_LOG.txt
CMakeLists.txt
FILE_LIST.md
MYSQL_SETUP.md
PROTOBUF_GUIDE.md
README.md
REDIS_GUIDE.md
SQLITE_GUIDE.md
VERIFICATION.md
VERSION_NOTES.md
include/chat_server.hpp
include/integration/file_transfer.hpp
include/integration/redis_client.hpp
include/integration/sqlite_client.hpp
include/mysql_database.hpp
sql/client_sqlite_schema.sql
src/chat_server.cpp
src/client.cpp
src/mysql_database.cpp
src/redis_client.cpp
src/server_main.cpp
src/sqlite_client.cpp
tests/test_proto.cpp
tests/test_redis_client.cpp
tests/test_sqlite_cache.cpp
```

## 新增文件

```text
CHANGE_MAP_V8_1_TO_V8_2.md
FILE_TRANSFER_GUIDE.md
MIGRATION_FROM_V8_1.md
MYSQL_RUN_THIS_V8_2.sql
MYSQL_UPGRADE_V8_1_TO_V8_2.sql
OFFICIAL_PROTOBUF_GUIDE.md
include/file_transfer_service.hpp
include/file_utils.hpp
include/proto_types.hpp
proto/file_transfer.proto
sql/005_create_file_transfers.sql
src/file_transfer_service.cpp
src/file_utils.cpp
tests/test_file_transfer_storage.cpp
tests/test_file_utils.cpp
```

## 删除文件

```text
include/proto_codec.hpp
src/proto_codec.cpp
```

# 关键源码修改位置

## CMakeLists.txt

从：

```text
自定义 proto_codec.cpp
```

改成：

```cmake
find_package(Protobuf REQUIRED)

add_library(chat_proto ...)

protobuf_generate(
    TARGET chat_proto
    LANGUAGE cpp
    IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/proto
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated
    APPEND_PATH
)

target_link_libraries(
    chat_proto
    PUBLIC protobuf::libprotobuf
)
```

同时加入：

```text
file_utils.cpp
file_transfer_service.cpp
test_file_utils
test_file_transfer_storage
```

## include/chat_server.hpp

`ClientSession` 新增：

```text
IncomingFileUpload
file_deliveries_in_progress
```

`ChatServer` 新增：

```text
FileTransferService
20 MiB limit
3072-byte chunk limit
file handlers
pending file delivery
receive acknowledgement
```

## src/chat_server.cpp

新增内部协议处理：

```text
FILE_BEGIN_PRIVATE
FILE_BEGIN_GROUP
FILE_CHUNK
FILE_END
FILE_ABORT
FILE_RECEIVED
FILE_RECEIVE_FAILED
```

核心新增函数：

```text
handle_file_begin_private
handle_file_begin_group
handle_file_chunk
handle_file_end
handle_file_abort
handle_file_received
handle_file_receive_failed
deliver_pending_files
deliver_file_to_user
abort_active_upload
reject_file_upload
```

`LOGIN` 和 `PENDING`：

```text
原 pending messages
+
pending files
```

## include/mysql_database.hpp / src/mysql_database.cpp

原聊天/群/好友 Protobuf：

```text
custom codec
```

全部改为：

```text
SerializeToString
ParseFromArray
official generated getters/setters
```

文件新增：

```text
StoredFileTransfer
add_file_transfer
pending_file_transfers
file_transfer_for_recipient
mark_file_transfer_delivered
```

## src/client.cpp

新增用户命令：

```text
SEND_FILE
SEND_GROUP_FILE
LOCAL_FILES
```

新增完整上传状态：

```text
PendingUpload
FILE_BEGIN
wait FILE_READY
FILE_CHUNK
FILE_END
FILE_UPLOAD_OK
```

新增完整下载状态：

```text
IncomingDownload
FILE_OFFER
FILE_DATA
FILE_DONE
local SHA-256
FILE_RECEIVED
```

失败时：

```text
FILE_RECEIVE_FAILED
```

使同一连接里仍可再次通过 `PENDING` 重试。

## Redis

`RedisUnreadCounts` 从：

```text
private_messages
group_messages
```

扩展为：

```text
private_messages
group_messages
private_files
group_files
```

支持 unread kind：

```text
private_file
group_file
```

## SQLite

新增：

```text
LocalFileTransfer
file_transfers table
cache_file_transfer
recent_file_transfers
stats.files
```

## server_main.cpp

新增可选服务器文件目录：

```bash
chat_server \
  <port> \
  <mysql.conf> \
  <redis.conf> \
  <workers> \
  <file_storage_root>
```

默认：

```text
data/server_files
```

## sql/005_create_file_transfers.sql

新增：

```text
file_transfers
file_transfer_deliveries
```

群文件的 `group_id`：

```text
ON DELETE SET NULL
```

所以群被解散后，发送时已经生成的待投递文件仍可以交付给原接收成员。

---

# Protobuf 删除/新增总结

删除：

```text
proto_codec.hpp
proto_codec.cpp
手写 varint/tag/length-delimited parser
```

保留并交给官方生成：

```text
friend_event.proto
chat_message.proto
group_message.proto
```

新增：

```text
file_transfer.proto
```

业务代码不再自己理解 Protobuf wire format。
