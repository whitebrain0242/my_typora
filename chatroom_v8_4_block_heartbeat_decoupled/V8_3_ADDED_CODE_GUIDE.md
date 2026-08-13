# v8.3 新增代码导读

这份文件专门把 v8.3 新增/替换的核心代码拆出来讲，便于从 v8.2 对照学习。

v8.3 有三个独立模块：

```text
A. TLS
B. 文件断点续传
C. MySQL MYSQL_STMT Prepared Statement
```

---

# A. TLS 模块

## A1. 新文件

```text
include/tls_config.hpp
include/minimuduo/net/TlsContext.hpp
src/minimuduo/net/TlsContext.cpp
config/tls_server.conf.example
config/tls_client.conf.example
scripts/generate_dev_tls_cert.sh
tests/test_tls_context.cpp
tests/test_tls_reactor.cpp
tests/run_tls_context_test.sh
tests/run_tls_reactor_test.sh
```

## A2. TLS 配置对象

```cpp
struct TlsServerConfig {
    bool enabled = true;
    std::string certificate_file =
        "config/tls/server.crt";
    std::string private_key_file =
        "config/tls/server.key";
};

struct TlsClientConfig {
    bool enabled = true;
    bool verify_peer = true;
    std::string ca_file =
        "config/tls/ca.crt";
    std::string server_name;
};
```

TLS 配置单独放在 `tls_config.hpp`，而不是让底层 `minimuduo` 依赖业务层 `config.hpp`。

## A3. SSL_CTX RAII

新增：

```cpp
class TlsServerContext;
class TlsClientContext;

struct SslDeleter {
    void operator()(SSL* ssl) const noexcept;
};

using SslPtr =
    std::unique_ptr<SSL, SslDeleter>;
```

服务器：

```cpp
context_ = SSL_CTX_new(
    TLS_server_method()
);

SSL_CTX_set_min_proto_version(
    context_,
    TLS1_2_VERSION
);

SSL_CTX_use_certificate_chain_file(...);
SSL_CTX_use_PrivateKey_file(...);
SSL_CTX_check_private_key(...);
```

客户端：

```cpp
context_ = SSL_CTX_new(
    TLS_client_method()
);

SSL_CTX_set_verify(
    context_,
    SSL_VERIFY_PEER,
    nullptr
);

SSL_CTX_load_verify_locations(...);
```

对 IP 地址：

```cpp
X509_VERIFY_PARAM_set1_ip_asc(
    parameters,
    peerIdentity.c_str()
);
```

对 DNS 名：

```cpp
SSL_set1_host(
    ssl.get(),
    peerIdentity.c_str()
);

SSL_set_tlsext_host_name(
    ssl.get(),
    peerIdentity.c_str()
);
```

因此默认不是“只加密但不验证服务器”，而是：

```text
TLS encryption
+
CA verification
+
hostname/IP verification
```

## A4. TLS 接入 TcpConnection

`TcpConnection` 构造函数新增：

```cpp
std::shared_ptr<TlsServerContext> tlsContext
```

新增状态：

```cpp
SslPtr ssl_;
bool tlsHandshakeComplete_;
bool applicationEstablished_;
bool tlsWriteBlockedOnRead_;
bool tlsReadBlockedOnWrite_;
```

连接建立后：

```cpp
void TcpConnection::connectEstablished() {
    setState(State::kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    if (tlsEnabled()) {
        driveTlsHandshake();
    } else {
        notifyApplicationEstablished();
    }
}
```

关键点是：

```text
TCP connected
!=
ChatServer connected
```

TLS 模式下必须：

```text
TCP accept
↓
SubReactor
↓
SSL_accept
↓
TLS handshake complete
↓
ChatServer::on_connection
```

所以登录、密码、聊天消息不会在握手之前以明文进入业务协议。

## A5. 非阻塞 TLS 握手

服务器连接 socket 本来就是 non-blocking。

因此不能：

```cpp
while (SSL_accept(...) != 1) {
    // busy wait
}
```

现在通过 epoll 事件推进：

```cpp
const int result =
    SSL_accept(ssl_.get());

const int sslError =
    SSL_get_error(
        ssl_.get(),
        result
    );

if (sslError == SSL_ERROR_WANT_READ) {
    channel_->enableReading();
}

if (sslError == SSL_ERROR_WANT_WRITE) {
    channel_->enableWriting();
}
```

这保持了 Reactor 的基本原则：

```text
一个 SubReactor
不会因为某一个 TLS handshake
阻塞其他连接
```

## A6. TLS 数据读取

原：

```cpp
inputBuffer_.readFd(socketFd_, ...);
```

TLS：

```cpp
const int result =
    SSL_read(
        ssl_.get(),
        buffer,
        sizeof(buffer)
    );

inputBuffer_.append(
    buffer,
    result
);
```

上层依然收到同一个：

```cpp
Buffer*
```

所以：

```text
ChatServer::on_message
命令解析
群组
Redis
MySQL
```

都不需要知道 TLS 细节。

## A7. TLS 数据发送

业务层仍然：

```cpp
connection->send(message);
```

连接属于当前 SubReactor 时：

```text
send
↓
outputBuffer
↓
SSL_write
↓
kernel socket
```

跨 SubReactor：

```text
business thread / another Reactor
↓
TcpConnection::send()
↓
runInLoop / queueInLoop
↓
eventfd
↓
owner SubReactor
↓
SSL_write
```

原来的线程归属规则没有被 TLS 打破。

---

# B. 断点续传模块

本版同时实现：

```text
上传断点续传
下载断点续传
```

不是只实现一个方向。

## B1. Protobuf resume sidecar

`proto/file_transfer.proto` 新增：

```proto
message FileUploadResumeState {
  FileTransferMetadata metadata = 1;
  repeated string recipient_usernames = 2;
}
```

服务端上传中间状态：

```text
data/server_files/tmp/
├── <token>.part
└── <token>.resume.pb
```

其中：

```text
.part
= 已经收到的原始文件前缀

.resume.pb
= official Protobuf
  文件 identity + 原始 recipients snapshot
```

## B2. begin_or_resume_upload

原 v8.2：

```cpp
begin_upload(token, temp_path)
```

v8.3：

```cpp
bool begin_or_resume_upload(
    const FileUploadResumeState& requested,
    FileUploadResumeState& persisted,
    std::filesystem::path& temp_path,
    std::uint64_t& accepted_offset,
    std::string& error
);
```

如果第一次上传：

```text
创建 .part
写 .resume.pb
offset = 0
```

如果已有 checkpoint：

```text
读取 .resume.pb
↓
比较 token / sender / target / file / size / SHA
↓
读取 .part 实际长度
↓
accepted_offset = file_size(.part)
```

服务器返回：

```text
FILE_READY <token> <accepted_offset>
```

## B3. 客户端从 offset 继续上传

客户端收到：

```text
FILE_READY token 3145728
```

执行：

```cpp
input.seekg(
    static_cast<std::streamoff>(
        start_offset
    ),
    std::ios::beg
);
```

然后：

```text
FILE_CHUNK token 3145728 ...
FILE_CHUNK token 3148800 ...
...
```

不再从 0 重传。

## B4. 上传任务写入 SQLite

客户端发送文件之前先保存：

```text
pending_uploads
```

字段：

```text
account_username
transfer_token
scope
target
source_path
file_name
file_size
sha256_hex
created_at_unix_ms
```

只有收到：

```text
FILE_UPLOAD_OK
```

才删除该任务。

客户端崩溃/退出后：

```text
SQLite task 仍在
```

下一次 LOGIN：

```text
load pending_uploads
↓
重新 FILE_BEGIN
↓
服务器返回 offset
↓
继续上传
```

也可以手动：

```text
RESUME_UPLOADS
```

## B5. 为什么 resume 前再次计算 SHA

恢复上传前客户端会检查：

```text
源文件还存在？
size 是否相同？
SHA-256 是否相同？
```

代码：

```cpp
if (!upload_source_matches(
        upload,
        error
    )) {
    ...
}
```

防止：

```text
昨天上传的是 A.zip 前 5MB
今天同一路径已经被替换成 B.zip
却继续把 B.zip 后半段拼到 A.zip 前半段
```

## B6. 群文件 recipient snapshot

群文件第一次开始上传时保存：

```text
recipient_usernames
```

恢复上传时不重新生成最终接收人。

这样：

```text
上传到 40%
↓
网络断开
↓
群成员发生变化
↓
恢复
```

仍然以“第一次发送时的目标成员”为 delivery snapshot。

但发送人恢复时仍要通过当前群成员权限检查。

## B7. 下载断点续传

服务端先只发送 metadata：

```text
FILE_OFFER
```

客户端检查本地：

```text
downloads/<user>/<id>_<name>.part
```

以及 SQLite：

```text
partial_downloads
```

然后：

```text
local_offset = filesystem::file_size(.part)
```

回复：

```text
FILE_RESUME_REQUEST <transfer_id> <local_offset>
```

服务器：

```cpp
input.seekg(
    static_cast<std::streamoff>(
        start_offset
    ),
    std::ios::beg
);
```

并返回：

```text
FILE_RESUME_START <id> <offset>
FILE_DATA <id> <offset> ...
...
FILE_DONE <id>
```

## B8. partial_downloads

SQLite 新增：

```text
partial_downloads
```

保存：

```text
server_transfer_id
scope
sender
group
file_name
temp_path
expected file_size
expected sha256
```

offset 不单独存 SQLite。

offset 直接取：

```cpp
std::filesystem::file_size(
    download.temp_path
);
```

这样减少：

```text
SQLite offset = 5MB
实际 .part = 4MB
```

这类双状态不一致。

## B9. 最终 SHA

断点续传没有降低完整性要求。

下载完成：

```text
.part exact size
↓
full SHA-256
↓
rename final
↓
SQLite file record
↓
FILE_RECEIVED id sha
↓
MySQL delivered
```

如果 `.part` 中间已经损坏：

```text
final SHA mismatch
↓
delete corrupt part
↓
remove partial_download
↓
next PENDING starts from 0
```

---

# C. MySQL Prepared Statement 模块

## C1. 删除的模式

v8.2 的数据库代码大量类似：

```cpp
const std::string sql =
    "SELECT ... username='" +
    escape(username) +
    "' LIMIT " +
    std::to_string(count);
```

v8.3 不再使用这种写法。

源码检查要求：

```text
mysql_real_escape_string = 0
escape()                 = 0
mysql_query              = 0
mysql_real_query         = 0
std::to_string in mysql_database.cpp = 0
```

## C2. MYSQL_STMT RAII

新增：

```cpp
class Statement final {
public:
    explicit Statement(
        MYSQL* connection
    )
        : statement_(
              mysql_stmt_init(connection)
          ) {}

    ~Statement() {
        if (statement_ != nullptr) {
            mysql_stmt_close(statement_);
        }
    }

    bool prepare(
        const char* sql,
        std::string& error
    );

    MYSQL_STMT* get() const noexcept;

private:
    MYSQL_STMT* statement_;
};
```

生命周期：

```text
mysql_stmt_init
↓
mysql_stmt_prepare
↓
bind
↓
execute/fetch
↓
mysql_stmt_close
```

## C3. 参数类型

新增：

```cpp
struct SqlParam {
    enum class Kind {
        Text,
        Blob,
        Unsigned64,
        Signed64,
        Null
    };
};
```

封装：

```cpp
SqlParam::text(...)
SqlParam::blob(...)
SqlParam::u64(...)
SqlParam::i64(...)
SqlParam::nullValue()
```

于是：

```text
username → MYSQL_TYPE_STRING
protobuf BLOB → MYSQL_TYPE_BLOB
id/file size → MYSQL_TYPE_LONGLONG unsigned
timestamp → MYSQL_TYPE_LONGLONG signed
SQL NULL → MYSQL_TYPE_NULL
```

## C4. 标准 INSERT

```cpp
static constexpr const char* sql =
    "INSERT INTO users("
    "username,password_hash"
    ") VALUES(?,?)";

return execute_prepared(
    connection_,
    sql,
    {
        SqlParam::text(username),
        SqlParam::text(password_hash)
    },
    nullptr,
    nullptr,
    error
);
```

## C5. 标准 SELECT

```cpp
static constexpr const char* sql =
    "SELECT password_hash "
    "FROM users "
    "WHERE username=? "
    "LIMIT 1";

Rows rows;

query_prepared(
    connection_,
    sql,
    {
        SqlParam::text(username)
    },
    rows,
    error
);
```

## C6. BLOB

官方 Protobuf：

```cpp
std::string bytes;

payload.SerializeToString(
    &bytes
);
```

绑定：

```cpp
SqlParam::blob(
    std::move(bytes)
)
```

不需要：

```text
escape binary bytes
UNHEX
hex string
```

## C7. LIMIT 也绑定

```cpp
"ORDER BY id DESC LIMIT ?"
```

参数：

```cpp
SqlParam::u64(count)
```

所以 count 也没有拼接到 SQL。

## C8. NULL

例如公共消息没有 recipient：

```cpp
const SqlParam recipient =
    payload.recipient_username().empty()
        ? SqlParam::nullValue()
        : SqlParam::text(
              payload.recipient_username()
          );
```

SQL 仍固定：

```sql
VALUES(?,?,?,?,?)
```

## C9. 事务不再拼 SQL

没有：

```cpp
execute("START TRANSACTION")
execute("COMMIT")
execute("ROLLBACK")
```

而是 MySQL C API：

```cpp
mysql_autocommit(
    connection_,
    0
);

mysql_commit(
    connection_
);

mysql_rollback(
    connection_
);
```

## C10. 回归检查

新增：

```text
tests/check_mysql_prepared.sh
```

CTest：

```text
mysql_prepared_statement_contract
```

如果以后重新出现：

```text
mysql_query
mysql_real_query
mysql_real_escape_string
std::to_string(...)
escape(...)
```

数据库合约测试会直接失败。

---

# D. 最重要的修改文件

```text
TLS:
include/tls_config.hpp
include/minimuduo/net/TlsContext.hpp
src/minimuduo/net/TlsContext.cpp
include/minimuduo/net/TcpConnection.hpp
src/minimuduo/net/TcpConnection.cpp
include/minimuduo/net/TcpServer.hpp
src/minimuduo/net/TcpServer.cpp
src/server_main.cpp
src/client.cpp

Resume:
proto/file_transfer.proto
include/file_transfer_service.hpp
src/file_transfer_service.cpp
include/chat_server.hpp
src/chat_server.cpp
include/integration/sqlite_client.hpp
src/sqlite_client.cpp
src/client.cpp

Prepared MySQL:
include/mysql_database.hpp
src/mysql_database.cpp
tests/check_mysql_prepared.sh

Build/tests:
CMakeLists.txt
tests/test_tls_context.cpp
tests/test_tls_reactor.cpp
tests/test_file_transfer_storage.cpp
tests/test_sqlite_cache.cpp
tests/test_proto.cpp
```

另外最终包包含：

```text
SOURCE_CHANGES_V8_2_TO_V8_3.patch
```

它只包含源码/配置/测试/SQL 的 unified diff，可以直接逐行看 v8.2 → v8.3 的代码变化。
