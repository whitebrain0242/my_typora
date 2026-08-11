# v8.2 Version Notes

## Added

- private file upload
- private offline file delivery
- group file upload
- group offline file delivery
- automatic login/PENDING file delivery
- server file storage
- SHA-256 sender/server/receiver validation
- random transfer token
- MySQL file metadata
- per-recipient file delivery
- Redis `private_file` / `group_file` unread
- SQLite file records
- `LOCAL_FILES`
- `FileTransferService` disk-read worker pool
- `proto/file_transfer.proto`
- official `protoc` build generation
- official `libprotobuf` linkage
- official Protobuf migration compatibility test
- file utility test
- file storage finalize test

## Removed

- `include/proto_codec.hpp`
- `src/proto_codec.cpp`
- hand-written protobuf varint/tag parser

## Preserved

- v7.3 business lineage
- Main/Sub Reactor
- friend management
- group management
- message histories
- offline private/group messages
- Redis Presence
- SQLite message cache

## Current limitations

- no resume-from-offset
- max file 20 MiB
- server files have no automatic expiry/GC
- one upload at a time per client connection
- file chunks use Base64 text framing for protocol compatibility
