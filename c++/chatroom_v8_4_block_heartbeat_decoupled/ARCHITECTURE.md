# chatroom v8.4 Architecture

详细版本：

```text
CLIENT_ARCHITECTURE_V8_4.md
SERVER_ARCHITECTURE_V8_4.md
```

总览：

```text
Client
┌──────────────────────────────────────────┐
│ ClientApp                                │
│ ├── TlsClientTransport                   │
│ ├── ClientHeartbeat                      │
│ ├── FileTransfer                         │
│ ├── LocalCommands                        │
│ ├── MessageCache                         │
│ └── SqliteClient                         │
└────────────────┬─────────────────────────┘
                 │ TLS/TCP
                 ▼
Server
┌──────────────────────────────────────────┐
│ MainReactor → TcpServer                  │
│                  │                       │
│                  ▼                       │
│            SubReactors                   │
│                  │                       │
│          TcpConnection/TLS               │
│                  │                       │
│          ServerCommandRouter             │
│                  │                       │
│              ChatServer                  │
│     ┌────────┬────┼─────┬──────────┐     │
│     ▼        ▼    ▼     ▼          ▼     │
│ Registry  Policy Heartbeat Files   Redis │
│               │             │            │
│               ▼             ▼            │
│             MySQL          disk           │
└──────────────────────────────────────────┘
```

TCP 存活：

```text
application PING/PONG
+
Linux TCP keepalive
```

Direct-message permission：

```text
user exists
↓
friendship
↓
friend_blocks
↓
private message/file allowed
```
