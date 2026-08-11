# Official Protobuf Migration Guide

## v8.1

旧文件：

```text
include/proto_codec.hpp
src/proto_codec.cpp
```

自己实现了：

```text
varint
field tag
length-delimited
skip unknown field
serialize_*
parse_*
```

## v8.2

以上两个文件已经删除。

新增：

```text
include/proto_types.hpp
```

它只 include 官方 `protoc` 生成的头文件：

```text
friend_event.pb.h
chat_message.pb.h
group_message.pb.h
file_transfer.pb.h
```

并提供类型别名，降低业务层迁移量。

## CMake

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

`.pb.cc/.pb.h` 不提交到源码树，而是在 build directory 由官方 compiler 生成。

## MySQL serialize

旧：

```cpp
std::string bytes =
    serialize_chat_message(payload);
```

新：

```cpp
std::string bytes;

if (!payload.SerializeToString(&bytes)) {
    ...
}
```

## MySQL parse

旧：

```cpp
parse_chat_message(bytes, payload);
```

新：

```cpp
payload.ParseFromArray(
    row[1],
    static_cast<int>(lengths[1])
);
```

## 构造 message

旧：

```cpp
ChatMessagePayload payload;
payload.type = ChatMessageType::Private;
payload.sender_username = sender;
```

新：

```cpp
ChatMessagePayload payload;
payload.set_type(chatroom::v7::PRIVATE);
payload.set_sender_username(sender);
```

## FriendEvent

使用官方生成 enum：

```cpp
chatroom::v7::FRIEND_REQUEST_SENT
chatroom::v7::FRIEND_REQUEST_ACCEPTED
chatroom::v7::FRIEND_REQUEST_REJECTED
chatroom::v7::FRIEND_REMOVED
```

## FileTransfer

新增：

```text
proto/file_transfer.proto
```

MySQL metadata 直接保存：

```cpp
FileTransferMetadata metadata;

metadata.set_file_name(...);
metadata.set_file_size(...);

metadata.SerializeToString(&bytes);
```

## 历史兼容

原 v7/v8 的：

```text
friend_event.proto
chat_message.proto
group_message.proto
```

没有改变字段号和 wire 类型。

因此历史 BLOB 不需要重新生成。

`tests/test_proto.cpp` 还拿 v7.x 已知 fixed bytes 交给官方类解析，防止迁移时无意破坏兼容。

## 安装

```bash
sudo apt install protobuf-compiler libprotobuf-dev
```

源码本身不包含 `protoc`、Protobuf header 或生成文件。
