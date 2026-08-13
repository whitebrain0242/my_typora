# v8.4 代码解耦说明

## 服务端新增边界

```text
client_session.hpp
    Session data only

command_router
    command name → handler

online_user_registry
    username → weak connection

direct_message_policy
    direct-message permission

heartbeat_manager
    connection liveness

chat_server_routes.cpp
    route composition

chat_server_blocking.cpp
    block commands

chat_server_heartbeat.cpp
    heartbeat protocol endpoint
```

## 客户端新增边界

```text
client.cpp
    bootstrap only

ClientApp
    orchestration/poll

TlsClientTransport
    TCP + TLS lifecycle

ClientHeartbeat
    heartbeat protocol/timeouts

ClientState
    data-only state

client_file_transfer
    FILE_* + resumable transfer

client_local_commands
    LOCAL_* / SEND_*

client_message_cache
    server text → SQLite records

SqliteClient
    local persistence
```

## 关键原则

```text
状态和行为分离
通信和持久化分离
命令路由和业务实现分离
心跳和聊天业务分离
文件协议和普通聊天文本分离
在线表和 ChatServer 分离
权限判断和具体 handler 分离
```

## 保持不变的功能语义

v8.3 已有：

```text
注册/登录
好友
群组
私聊
群聊
历史
离线消息
Redis
SQLite
官方 Protobuf
TLS
在线/离线文件
断点续传
```

都继续保留。

本次重构主要改变：

```text
代码在哪里
谁拥有状态
模块之间如何依赖
```

而不是重新设计这些已有功能的用户协议。
