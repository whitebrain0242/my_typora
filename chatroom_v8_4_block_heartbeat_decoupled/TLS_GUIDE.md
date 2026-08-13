# TLS + Heartbeat Guide — v8.4

## 模型

v8.3 使用：

```text
server-authenticated TLS
```

也就是：

```text
client verifies server
server does not require client certificate
```

不是 mTLS。

默认：

```text
minimum TLS 1.2
TLS 1.3 when OpenSSL supports it
certificate verification ON
```

## 生成开发证书

```bash
./scripts/generate_dev_tls_cert.sh
```

默认生成：

```text
config/tls/
├── ca.crt
├── ca.key
├── server.crt
├── server.key
├── server.csr
└── server.ext
```

`server.crt` SAN：

```text
DNS:localhost
IP:127.0.0.1
```

私钥：

```text
ca.key
server.key
```

不要提交到仓库或发送给别人。

## 配置

服务端：

```bash
cp config/tls_server.conf.example \
   config/tls_server.conf
```

客户端：

```bash
cp config/tls_client.conf.example \
   config/tls_client.conf
```

服务端示例：

```text
enabled=true
certificate_file=config/tls/server.crt
private_key_file=config/tls/server.key
```

客户端：

```text
enabled=true
verify_peer=true
ca_file=config/tls/ca.crt
server_name=
```

客户端连接：

```bash
./build/chat_client \
  127.0.0.1 \
  9000 \
  data/alice.db \
  downloads \
  config/tls_client.conf
```

如果 `server_name` 留空：

```text
验证命令行中的 127.0.0.1
```

## Reactor 集成

TLS handshake 不在 MainReactor 做。

```text
MainReactor
accept TCP
↓
round-robin
↓
SubReactor
SSL_accept(nonblocking)
↓
WANT_READ / WANT_WRITE
↓
epoll
↓
handshake complete
↓
ChatServer::on_connection
```

## 测试

```text
tls_context_verified
tls_reactor_echo
```

第二个测试是真正让：

```text
OpenSSL client
→ TLS
→ TcpServer
→ SubReactor
→ TcpConnection::SSL_read
→ callback
→ TcpConnection::SSL_write
→ client
```

完成 echo。


## v8.4 心跳与 TLS

应用心跳：

```text
PING/PONG
```

位于 TLS application data 内。

因此：

```text
heartbeat
login
chat
file protocol
```

都受到同一 TLS channel 保护。

详细见：

```text
HEARTBEAT_GUIDE.md
```
