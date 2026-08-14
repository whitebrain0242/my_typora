# v8.4 TCP/TLS 心跳检测设计

## 两层检测

### 应用层

```text
PING <nonce>
PONG <nonce>
```

默认：

```text
server PING interval = 20s
server timeout = 60s
client timeout = 75s
scan interval = 1s
```

### Kernel TCP keepalive

Linux accepted/client sockets：

```text
SO_KEEPALIVE=1
TCP_KEEPIDLE=60
TCP_KEEPINTVL=15
TCP_KEEPCNT=3
```

## 为什么两者都要

应用心跳：

```text
时间语义明确
容易测试
可以快速清理业务 session
```

Kernel keepalive：

```text
即使应用 heartbeat 线程异常
内核仍有网络级 fallback
```

## TLS

PING/PONG 在：

```text
TLS application data
```

中传输。

网络抓包不会看到明文：

```text
PING 123
LOGIN alice ...
MSG ...
```

## 线程安全

HeartbeatManager thread：

```text
不直接 read/write socket
不直接 SSL_write
```

只调用线程安全的：

```text
TcpConnection::send
TcpConnection::forceClose
```

最终真正网络操作仍回 connection 所属 SubReactor。
