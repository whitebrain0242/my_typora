# v8.4 Version Notes

## Features

新增：

```text
BLOCK_FRIEND
UNBLOCK_FRIEND
BLOCKED_FRIENDS
```

屏蔽范围：

```text
private text
private file
offline private text delivery
offline private file delivery
```

保持：

```text
public/group communication
friendship
history
presence
```

## Heartbeat

新增：

```text
HeartbeatManager
server PING every 20s
server timeout 60s
client auto PONG
client timeout 75s
```

并启用 Linux：

```text
SO_KEEPALIVE
TCP_KEEPIDLE
TCP_KEEPINTVL
TCP_KEEPCNT
```

## Server decoupling

新增：

```text
ClientSession module
ServerCommandRouter
OnlineUserRegistry
DirectMessagePolicy
HeartbeatManager
chat_server_routes.cpp
chat_server_blocking.cpp
chat_server_heartbeat.cpp
```

## Client decoupling

新增：

```text
ClientApp
ClientState
TlsClientTransport
ClientHeartbeat
client_file_transfer
client_local_commands
client_message_cache
client_common
```

`src/client.cpp` 现在只做 bootstrap。

## Database

新增 MySQL：

```text
friend_blocks
```

所有新数据库访问仍是：

```text
MYSQL_STMT Prepared Statement
```

## New tests

```text
tcp_application_heartbeat_timeout
client_heartbeat_auto_pong
server_command_router
friend_block_persistence_contract
```
