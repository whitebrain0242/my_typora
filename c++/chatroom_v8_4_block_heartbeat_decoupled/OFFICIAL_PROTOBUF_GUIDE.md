# Official Protobuf Guide — v8.4

项目继续坚持：

```text
official protoc + libprotobuf only
```

没有恢复任何自定义 wire codec。

CMake：

```cmake
find_package(Protobuf REQUIRED)
protobuf_generate(...)
protobuf::libprotobuf
```

已有：

```text
friend_event.proto
chat_message.proto
group_message.proto
file_transfer.proto
```

v8.3 在 `file_transfer.proto` 新增：

```proto
message FileUploadResumeState {
  FileTransferMetadata metadata = 1;
  repeated string recipient_usernames = 2;
}
```

用途：

```text
server tmp/<token>.resume.pb
```

这个 sidecar 保存断点上传 metadata 和第一次上传的 recipient snapshot。

代码使用：

```cpp
state.SerializeToString(&bytes);
state.ParseFromString(bytes);
```

不是手工解析 Protobuf wire format。

需要：

```bash
sudo apt install protobuf-compiler libprotobuf-dev
```
