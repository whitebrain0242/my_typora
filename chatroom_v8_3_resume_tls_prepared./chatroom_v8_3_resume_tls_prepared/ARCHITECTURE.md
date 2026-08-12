# v8.3 Architecture

```text
                        MainReactor
                    main thread + epoll
                            │
                         accept
                            │
             ┌──────────────┴──────────────┐
             ▼                             ▼
       SubReactor 0                  SubReactor N
       epoll + eventfd               epoll + eventfd
             │                             │
             │ nonblocking TLS             │
             │ SSL_accept/read/write       │
             └───────────┬─────────────────┘
                         ▼
                     ChatServer
                 ┌───────┼────────┐
                 ▼       ▼        ▼
             MySQL     Redis   FileTransferService
          MYSQL_STMT   cache     disk workers
                 │
                 ▼
        durable delivery/history
```

客户端：

```text
TCP
↓
TLS verify + SSL_connect
↓
SSL_read / SSL_write
↓
text command protocol
↓
SQLite
├── messages
├── file_transfers
├── pending_uploads
└── partial_downloads
```

断点上传：

```text
SQLite pending_upload
+
server .part
+
server official-Protobuf .resume.pb
```

断点下载：

```text
MySQL pending delivery
+
client SQLite partial_download
+
client .part
```

业务层不知道 TLS record 格式。

网络层不知道聊天命令、MySQL schema 和 SQLite schema。

MySQL 层不知道 TLS。
