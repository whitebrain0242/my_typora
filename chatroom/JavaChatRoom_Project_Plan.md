# JavaChatRoom 聊天室项目需求文档、架构设计与两周开发计划

## 0. 项目定位

项目名称：`JavaChatRoom`

项目类型：Java C/S 双端聊天室

开发人数：2 人

总周期：14 天

推荐技术路线：

- 服务端：Java 17 + Netty 4.1.x + MySQL 8.x + Maven + Log4j2 + JUnit 5
- 客户端：Java 17 CLI 客户端，后期可扩展 JavaFX 图形界面
- 网络协议：TCP 长连接 + 自定义 JSON 消息协议
- 数据存储：MySQL，消息体、请求体、响应体使用 JSON 序列化/反序列化
- I/O 多路复用：Netty NIO，底层基于 Selector/Epoll 事件模型
- 提高功能建议选择：
  1. 验证码登录/注册/密码找回：采用邮箱验证码
  2. TLS 通信加密：Netty SSL/TLS
  3. 文件发送断点续传
  4. 主从 Reactor 高性能服务器模型：Netty bossGroup + workerGroup

> 两周内建议优先保证：登录注册、好友、群聊、实时聊天、离线消息、心跳、日志、MySQL 存储、基础文件发送。GUI 和超高并发作为可选扩展。

---

## 1. 项目前需要掌握的技术

### 1.1 Java 基础与工程能力

必须掌握：

- Java 17 基础语法、面向对象、异常处理、集合框架
- 泛型、枚举、接口、抽象类
- 多线程基础：线程池、并发集合、锁、volatile、synchronized
- Maven 项目结构和依赖管理
- IDEA 创建 Maven 多模块项目
- JUnit 5 单元测试
- Log4j2 日志配置

需要重点理解：

- `try-catch-finally` 如何保证资源释放
- `Optional`、`LocalDateTime`、`UUID` 的常见用法
- `Map<Long, Channel>` 维护在线用户连接
- Java 对象和 JSON 字符串之间如何转换

### 1.2 网络编程与 Netty

必须掌握：

- TCP 与 UDP 区别
- TCP 长连接、粘包、半包问题
- 心跳检测机制
- Channel、ChannelHandler、ChannelPipeline、EventLoop
- Netty 编码器和解码器
- Netty 的 bossGroup / workerGroup 主从 Reactor 模型
- 客户端连接、断线重连、异常关闭

建议学习顺序：

1. Java Socket 基础
2. Java NIO：Channel、Buffer、Selector
3. Netty 入门 EchoServer
4. Netty 自定义协议
5. Netty 心跳检测 IdleStateHandler
6. Netty 文件传输 ChunkedWriteHandler

### 1.3 数据库 MySQL

必须掌握：

- 表、主键、外键、索引
- CRUD：`insert`、`delete`、`update`、`select`
- 事务：好友添加、加群审批、消息写入需要保证一致性
- 唯一约束：用户名、邮箱、手机号、群成员关系
- 分页查询：历史消息记录
- JDBC 或 MyBatis 的使用

推荐使用：MyBatis + HikariCP 连接池。

### 1.4 数据结构与业务模型

需要掌握：

- HashMap：维护在线用户表
- ConcurrentHashMap：线程安全在线连接管理
- Queue：离线通知队列思想
- Set：好友关系、群成员集合
- 状态机：好友申请、加群申请、消息状态

### 1.5 Git 与 GitHub 协作

必须掌握：

- `git init`、`git add`、`git commit`
- `git branch`、`git checkout -b`
- `git push`、`git pull`
- Pull Request 合并代码
- `.gitignore` 配置
- README 编写
- Issue 拆分任务

建议协作方式：

- `main`：稳定可运行版本
- `dev`：每日集成版本
- `feature/account`：账号模块
- `feature/friend`：好友模块
- `feature/group`：群组模块
- `feature/chat`：聊天模块
- `feature/file-transfer`：文件传输模块

---

## 2. 项目总体需求文档

### 2.1 项目目标

实现一个支持多客户端同时在线访问的 Java C/S 聊天系统。系统应支持用户注册、登录、注销、好友管理、私聊、群聊、离线消息、文件传输、心跳检测、服务器日志、数据库持久化，并保证非法输入不会导致客户端或服务器崩溃。

### 2.2 用户角色

| 角色 | 权限 |
|---|---|
| 未登录用户 | 注册、登录、验证码登录、找回密码 |
| 普通用户 | 好友管理、私聊、群组申请、群聊、文件发送 |
| 群管理员 | 批准加群申请、移除普通群成员 |
| 群主 | 解散群、添加/删除管理员、移除成员 |
| 系统管理员，可选 | 查看日志、服务器状态 |

### 2.3 功能优先级

| 优先级 | 功能 |
|---|---|
| P0 必须完成 | 注册、登录、注销、好友添加/删除/查询、私聊、群创建、群聊、历史消息、离线消息、心跳、日志、MySQL 存储 |
| P1 推荐完成 | 邮箱验证码、密码加密、好友在线状态、屏蔽好友消息、加群审批、文件在线发送 |
| P2 提高功能 | TLS、断点续传、JavaFX GUI、服务端监控、Docker 打包 |

---

## 3. 详细功能设计

## 3.1 账号管理模块 Account

### 3.1.1 注册

功能描述：用户输入用户名、密码、邮箱或手机号，系统创建新账号。

业务规则：

- 用户名唯一
- 邮箱唯一
- 密码不能明文存储
- 注销过的用户名不能直接恢复旧数据
- 注册成功后默认状态为 `ACTIVE`

主要接口：

```java
public RegisterResponse register(RegisterRequest request)
```

方法职责：

- 校验用户名、密码、邮箱格式
- 检查用户名或邮箱是否已存在
- 对密码进行 BCrypt 哈希
- 写入 `users` 表
- 返回用户 ID 和注册结果

关键类：

```java
class AccountService {
    RegisterResponse register(RegisterRequest request);
    LoginResponse login(LoginRequest request);
    LogoutResponse logout(Long userId);
    boolean verifyPassword(String rawPassword, String encodedPassword);
    void changePassword(ResetPasswordRequest request);
}
```

### 3.1.2 登录

功能描述：用户输入账号密码，验证成功后建立在线状态。

业务规则：

- 禁止已注销用户登录
- 登录成功后绑定 `userId -> Channel`
- 登录成功后推送离线消息通知、离线文件通知、好友请求通知、加群申请通知

主要接口：

```java
public LoginResponse login(LoginRequest request)
```

方法职责：

- 查询用户是否存在
- 校验密码
- 更新最后登录时间
- 把用户加入在线用户管理器
- 返回 token 或 sessionId

### 3.1.3 注销账号

功能描述：用户主动注销账号。

业务规则：

- 注销不是物理删除用户数据
- 用户状态改为 `DELETED`
- 用户不能再登录
- 好友关系、群成员关系保留历史记录，但不允许继续使用

主要接口：

```java
public LogoutResponse logout(Long userId)
public DeleteAccountResponse deleteAccount(Long userId)
```

### 3.1.4 邮箱验证码登录/注册/密码找回，提高功能

主要接口：

```java
public void sendEmailCode(String email, VerifyCodeScene scene)
public boolean verifyEmailCode(String email, String code, VerifyCodeScene scene)
public void resetPassword(ResetPasswordRequest request)
```

方法职责：

- 生成 6 位验证码
- 保存验证码到 `verify_codes` 表
- 设置过期时间，例如 5 分钟
- 邮件发送失败时返回明确错误
- 验证码使用后标记为已使用

### 3.1.5 数据加密

实现内容：

- 密码：BCrypt 单向哈希
- 敏感配置：数据库密码不写死到代码中，放入 `application.yml`
- 通信加密：提高功能使用 TLS

---

## 3.2 好友管理模块 Friend

### 3.2.1 添加好友

功能描述：用户向另一个用户发送好友申请。

业务规则：

- 不能添加自己
- 不能添加不存在或已注销用户
- 已经是好友不能重复添加
- 已有待处理申请不能重复发送
- 对方在线时立即收到好友请求通知

主要接口：

```java
public FriendApplyResponse applyFriend(Long fromUserId, Long toUserId, String remark)
```

方法职责：

- 校验双方用户状态
- 检查好友关系是否已存在
- 写入 `friend_requests` 表
- 如果接收方在线，通过 Netty 推送通知

### 3.2.2 批准或拒绝好友申请

```java
public FriendApplyHandleResponse handleFriendRequest(Long requestId, Long operatorId, FriendRequestAction action)
```

方法职责：

- 校验申请是否存在
- 校验操作者是否为接收方
- 批准时写入双向好友关系
- 更新申请状态
- 通知申请人结果

### 3.2.3 删除好友

```java
public void deleteFriend(Long userId, Long friendId)
```

方法职责：

- 删除或软删除双方好友关系
- 删除后禁止双方私聊
- 可保留历史聊天记录

### 3.2.4 查询好友

```java
public List<FriendDTO> listFriends(Long userId)
public FriendDTO getFriendInfo(Long userId, Long friendId)
```

方法职责：

- 查询好友列表
- 查询在线状态
- 返回备注、头像、最近登录时间

### 3.2.5 显示好友在线状态

```java
public boolean isOnline(Long userId)
public List<Long> getOnlineFriendIds(Long userId)
```

实现方式：

- 服务端使用 `OnlineUserManager` 维护在线连接
- 好友列表返回时动态查询在线状态

### 3.2.6 禁止非好友私聊

```java
public boolean canPrivateChat(Long fromUserId, Long toUserId)
```

校验规则：

- 双方必须存在好友关系
- 双方账号状态必须正常
- 没有被屏蔽

### 3.2.7 屏蔽好友消息

```java
public void blockFriend(Long userId, Long friendId)
public void unblockFriend(Long userId, Long friendId)
public boolean isBlocked(Long receiverId, Long senderId)
```

业务规则：

- A 屏蔽 B 后，B 发给 A 的消息不实时推送
- 消息是否保存可以根据需求决定，建议保存但标记 `blocked = true`

---

## 3.3 群管理模块 Group

### 3.3.1 创建群组

```java
public GroupCreateResponse createGroup(Long ownerId, String groupName, String description)
```

方法职责：

- 创建 `groups` 记录
- 创建群成员记录，创建者角色为 `OWNER`
- 返回群 ID

### 3.3.2 解散群组

```java
public void dissolveGroup(Long ownerId, Long groupId)
```

业务规则：

- 只有群主可以解散群
- 解散后群状态为 `DISSOLVED`
- 解散后所有成员不能继续发送群消息

### 3.3.3 申请加入群组

```java
public GroupJoinApplyResponse applyJoinGroup(Long userId, Long groupId, String reason)
```

业务规则：

- 不能重复申请
- 已在群内不能申请
- 被移除成员是否允许再次申请由配置决定，建议允许但需要审批
- 群管理员和群主在线时收到申请通知

### 3.3.4 查看已加入群组

```java
public List<GroupDTO> listJoinedGroups(Long userId)
```

返回内容：

- 群 ID
- 群名称
- 群角色：OWNER / ADMIN / MEMBER
- 群人数
- 最后一条消息摘要，可选

### 3.3.5 退出群组

```java
public void quitGroup(Long userId, Long groupId)
```

业务规则：

- 普通成员和管理员可以退出
- 群主不能直接退出，必须先转让群主或解散群

### 3.3.6 查看群成员列表

```java
public List<GroupMemberDTO> listGroupMembers(Long userId, Long groupId)
```

校验规则：

- 只有群成员可以查看
- 已被移除用户不能查看

### 3.3.7 添加或删除群管理员

```java
public void addGroupAdmin(Long ownerId, Long groupId, Long targetUserId)
public void removeGroupAdmin(Long ownerId, Long groupId, Long targetUserId)
```

业务规则：

- 只有群主可以操作
- 目标用户必须是群成员
- 群主不能被降级

### 3.3.8 批准加群申请

```java
public void handleJoinGroupRequest(Long operatorId, Long requestId, GroupRequestAction action)
```

业务规则：

- 群主和管理员可以审批
- 批准后写入 `group_members`
- 拒绝后更新申请状态
- 通知申请人审批结果

### 3.3.9 移除群成员

```java
public void removeGroupMember(Long operatorId, Long groupId, Long targetUserId)
```

业务规则：

- 群主可以移除管理员和普通成员
- 管理员只能移除普通成员
- 不能移除群主
- 被移除用户不能继续发送群消息

### 3.3.10 群组聊天

```java
public SendGroupMessageResponse sendGroupMessage(Long senderId, Long groupId, String content, MessageType type)
```

校验规则：

- 发送者必须是群成员
- 群状态必须正常
- 被移除成员不能发送
- 消息写入数据库后再推送在线成员

---

## 3.4 聊天功能模块 Chat

### 3.4.1 查看历史消息记录

私聊：

```java
public PageResult<MessageDTO> listPrivateHistory(Long userId, Long friendId, int page, int size)
```

群聊：

```java
public PageResult<MessageDTO> listGroupHistory(Long userId, Long groupId, int page, int size)
```

功能要求：

- 按时间倒序或正序分页
- 私聊只能查看自己参与的消息
- 群聊只能查看自己所在群的消息

### 3.4.2 在线聊天

```java
public SendPrivateMessageResponse sendPrivateMessage(Long fromUserId, Long toUserId, String content, MessageType type)
```

执行流程：

1. 校验双方好友关系
2. 校验接收方是否屏蔽发送方
3. 消息写入数据库
4. 如果接收方在线，实时推送
5. 如果接收方离线，记录未读状态，待上线通知

### 3.4.3 离线消息通知

```java
public List<OfflineNotificationDTO> getOfflineNotifications(Long userId)
public void pushOfflineNotifications(Long userId)
```

功能要求：

- 用户上线后收到：未读私聊数量、未读群聊数量、离线文件数量、好友申请、加群申请
- 不一定一次推送所有历史消息，可以先推送摘要，再由客户端主动拉取

### 3.4.4 实时通知

通知类型：

- 好友请求
- 好友申请结果
- 私聊消息
- 群聊消息
- 加群申请
- 加群审批结果
- 被移出群通知
- 群解散通知
- 离线文件通知

统一方法：

```java
public void pushToUser(Long userId, ServerMessage<?> message)
public void pushToGroup(Long groupId, ServerMessage<?> message)
```

### 3.4.5 在线文件发送

```java
public FileUploadInitResponse initFileUpload(Long senderId, FileMetaDTO meta)
public void uploadFileChunk(Long senderId, String fileId, int chunkIndex, byte[] data)
public void completeFileUpload(Long senderId, String fileId)
public void notifyFileReceiver(Long receiverId, FileMessageDTO fileMessage)
```

功能要求：

- 文件元数据写入 `files` 表
- 文件内容保存到服务器本地 `storage/` 目录
- 发送完成后在消息表写入文件消息
- 接收方在线时收到文件通知

### 3.4.6 断点续传，提高功能

```java
public FileResumeInfo getResumeInfo(String fileId)
public void uploadFileChunk(String fileId, int chunkIndex, byte[] chunkData)
public boolean isChunkUploaded(String fileId, int chunkIndex)
public void mergeChunks(String fileId)
```

功能要求：

- 文件按固定大小切片，例如 1MB
- `file_chunks` 表记录每个分片上传状态
- 客户端断开后重新连接，先查询已上传分片
- 只补传未上传分片

---

## 3.5 心跳检测

要求：TCP 心跳检测不能单独创建线程。

推荐实现：使用 Netty `IdleStateHandler`。

服务端 Pipeline：

```java
pipeline.addLast(new IdleStateHandler(60, 0, 0, TimeUnit.SECONDS));
pipeline.addLast(new ServerHeartbeatHandler());
```

主要方法：

```java
public void userEventTriggered(ChannelHandlerContext ctx, Object evt)
public void handleHeartbeatPing(ChannelHandlerContext ctx, HeartbeatPing ping)
public void sendHeartbeatPong(ChannelHandlerContext ctx)
```

规则：

- 客户端每 30 秒发送一次 PING
- 服务端 60 秒未读到数据则认为连接异常
- 服务端关闭异常 Channel
- 用户下线后清理在线用户表

---

## 3.6 日志模块

使用 Log4j2。

日志内容：

- 服务端启动和关闭
- 用户登录、退出
- 网络连接建立、断开
- 私聊、群聊发送失败原因
- 数据库异常
- 文件上传失败
- 非法请求

主要类：

```java
class ServerLogger {
    void info(String msg);
    void warn(String msg);
    void error(String msg, Throwable e);
}
```

---

## 3.7 客户端 CLI 功能

客户端启动参数：

```bash
java -jar chat-client.jar --host 127.0.0.1 --port 9000
```

服务端启动参数：

```bash
java -jar chat-server.jar --host 0.0.0.0 --port 9000
```

客户端菜单：

```text
1. 注册
2. 登录
3. 验证码登录
4. 找回密码
5. 好友管理
6. 群组管理
7. 私聊
8. 群聊
9. 文件发送
10. 查看历史消息
11. 退出登录
0. 退出程序
```

---

## 4. 非功能需求

### 4.1 稳定性

- 所有客户端输入都要做校验
- 服务端不能因为单个非法请求崩溃
- 数据库异常需要返回错误响应
- 客户端断开后服务端及时清理连接
- JSON 反序列化失败时返回协议错误

### 4.2 并发能力

- 支持至少 100 个客户端同时连接
- 在线连接使用 `ConcurrentHashMap`
- 数据库访问使用连接池
- 群发消息不能阻塞 Netty I/O 线程，复杂任务交给业务线程池

### 4.3 安全性

- 密码不明文保存
- 登录失败次数限制，可选
- TLS 加密，可选提高
- 文件上传限制大小和类型
- 防止路径穿越，例如禁止文件名包含 `../`

### 4.4 可维护性

- 按模块拆分包结构
- 请求、响应、DTO 分离
- Service 负责业务逻辑
- DAO/Mapper 负责数据库操作
- Handler 只负责网络消息分发

---

## 5. 项目架构设计

## 5.1 总体架构

```text
+-------------------+          TCP/JSON           +-----------------------+
| Java CLI Client   | <-------------------------> | Netty Chat Server     |
| JavaFX Client 可选 |                              | Boss + Worker Reactor |
+-------------------+                              +-----------+-----------+
                                                                |
                                                                v
                                                     +---------------------+
                                                     | Service Layer       |
                                                     | Account/Friend/...  |
                                                     +----------+----------+
                                                                |
                                                                v
                                                     +---------------------+
                                                     | MyBatis Mapper DAO  |
                                                     +----------+----------+
                                                                |
                                                                v
                                                     +---------------------+
                                                     | MySQL Database      |
                                                     +---------------------+
```

### 5.2 Maven 多模块结构

```text
chatroom/
├── pom.xml
├── chat-common/
│   ├── src/main/java/com/chatroom/common/
│   │   ├── protocol/
│   │   ├── dto/
│   │   ├── enums/
│   │   ├── codec/
│   │   └── util/
├── chat-server/
│   ├── src/main/java/com/chatroom/server/
│   │   ├── ChatServerApplication.java
│   │   ├── netty/
│   │   ├── handler/
│   │   ├── service/
│   │   ├── mapper/
│   │   ├── entity/
│   │   ├── manager/
│   │   ├── config/
│   │   └── storage/
│   └── src/main/resources/
│       ├── application.yml
│       ├── mybatis-config.xml
│       └── log4j2.xml
├── chat-client/
│   ├── src/main/java/com/chatroom/client/
│   │   ├── ChatClientApplication.java
│   │   ├── netty/
│   │   ├── handler/
│   │   ├── command/
│   │   ├── view/
│   │   └── context/
└── docs/
    ├── requirement.md
    ├── database.sql
    ├── api_protocol.md
    └── user_manual.md
```

---

## 6. 开发环境与工具版本

| 工具 | 推荐版本 | 说明 |
|---|---:|---|
| JDK | 17 | 已满足 |
| IDEA | 2023+ / 2024+ / 2025+ | Community 也可以 |
| Maven | 3.9.x | 构建工具 |
| MySQL | 8.0.x | 数据库存储 |
| Netty | 4.1.x | 网络通信 |
| MyBatis | 3.5.x | ORM/SQL 映射 |
| HikariCP | 5.x | 数据库连接池 |
| Jackson | 2.17+ | JSON 序列化 |
| Log4j2 | 2.23+ | 日志 |
| JUnit | 5.x | 单元测试 |
| Git | 2.x | 版本控制 |
| GitHub | 免费账号即可 | 代码托管 |
| Postman | 可选 | 测试接口思想，不是必须 |

---

## 7. 编码规范

### 7.1 包名规范

```text
com.chatroom.common
com.chatroom.server
com.chatroom.client
```

### 7.2 命名规范

| 类型 | 规则 | 示例 |
|---|---|---|
| 类名 | 大驼峰 | `AccountService` |
| 方法名 | 小驼峰 | `sendPrivateMessage` |
| 常量 | 全大写下划线 | `MAX_FILE_SIZE` |
| 表名 | 小写下划线 | `friend_requests` |
| 字段名 | 小写下划线 | `created_at` |

### 7.3 Git Commit 规范

```text
feat: add user register service
fix: fix private message permission check
docs: update database design
test: add account service tests
refactor: split chat handler logic
```

---

## 8. 主要类设计

## 8.1 common 模块

### 8.1.1 协议消息类

```java
public class MessagePacket<T> {
    private String requestId;
    private Integer type;
    private Long timestamp;
    private T body;
}
```

职责：所有客户端和服务端通信都使用该包装类。

### 8.1.2 消息类型枚举

```java
public enum MessageCommand {
    REGISTER_REQ,
    REGISTER_RESP,
    LOGIN_REQ,
    LOGIN_RESP,
    PRIVATE_CHAT_REQ,
    PRIVATE_CHAT_RESP,
    GROUP_CHAT_REQ,
    GROUP_CHAT_RESP,
    HEARTBEAT_PING,
    HEARTBEAT_PONG,
    FILE_UPLOAD_INIT_REQ,
    FILE_CHUNK_REQ,
    ERROR_RESP
}
```

### 8.1.3 编解码器

```java
public class JsonPacketEncoder extends MessageToByteEncoder<MessagePacket<?>> {
    protected void encode(ChannelHandlerContext ctx, MessagePacket<?> msg, ByteBuf out);
}

public class JsonPacketDecoder extends ByteToMessageDecoder {
    protected void decode(ChannelHandlerContext ctx, ByteBuf in, List<Object> out);
}
```

职责：解决粘包半包问题，推荐采用：

```text
4 字节 length + JSON body
```

---

## 8.2 server 模块

### 8.2.1 启动类

```java
public class ChatServerApplication {
    public static void main(String[] args);
}
```

职责：读取 host、port、配置文件，启动 Netty 服务端。

### 8.2.2 Netty 服务端

```java
public class NettyChatServer {
    public void start(String host, int port);
    public void shutdown();
}
```

职责：

- 创建 bossGroup 和 workerGroup
- 配置 Pipeline
- 绑定端口
- 优雅关闭

### 8.2.3 服务端消息分发器

```java
public class ServerMessageDispatcher {
    public void dispatch(ChannelHandlerContext ctx, MessagePacket<?> packet);
}
```

职责：根据 `packet.type` 分发到对应业务 Handler。

### 8.2.4 在线用户管理

```java
public class OnlineUserManager {
    public void online(Long userId, Channel channel);
    public void offline(Long userId);
    public boolean isOnline(Long userId);
    public Channel getChannel(Long userId);
    public void sendToUser(Long userId, MessagePacket<?> packet);
    public Set<Long> getOnlineUserIds();
}
```

### 8.2.5 账号服务

```java
public class AccountService {
    public RegisterResponse register(RegisterRequest request);
    public LoginResponse login(LoginRequest request, Channel channel);
    public void logout(Long userId);
    public void deleteAccount(Long userId);
    public void sendVerifyCode(String email, VerifyCodeScene scene);
    public void resetPassword(ResetPasswordRequest request);
}
```

### 8.2.6 好友服务

```java
public class FriendService {
    public void applyFriend(Long fromUserId, Long toUserId, String remark);
    public void handleFriendRequest(Long requestId, Long userId, FriendRequestAction action);
    public void deleteFriend(Long userId, Long friendId);
    public List<FriendDTO> listFriends(Long userId);
    public void blockFriend(Long userId, Long friendId);
    public void unblockFriend(Long userId, Long friendId);
    public boolean areFriends(Long userId, Long friendId);
}
```

### 8.2.7 群组服务

```java
public class GroupService {
    public Long createGroup(Long ownerId, String groupName, String description);
    public void dissolveGroup(Long ownerId, Long groupId);
    public void applyJoinGroup(Long userId, Long groupId, String reason);
    public void handleJoinRequest(Long operatorId, Long requestId, GroupRequestAction action);
    public void quitGroup(Long userId, Long groupId);
    public void addAdmin(Long ownerId, Long groupId, Long targetUserId);
    public void removeAdmin(Long ownerId, Long groupId, Long targetUserId);
    public void removeMember(Long operatorId, Long groupId, Long targetUserId);
    public List<GroupDTO> listJoinedGroups(Long userId);
    public List<GroupMemberDTO> listMembers(Long userId, Long groupId);
    public boolean isGroupMember(Long userId, Long groupId);
}
```

### 8.2.8 聊天服务

```java
public class ChatService {
    public void sendPrivateMessage(Long fromUserId, Long toUserId, String content, MessageType type);
    public void sendGroupMessage(Long fromUserId, Long groupId, String content, MessageType type);
    public PageResult<MessageDTO> listPrivateHistory(Long userId, Long friendId, int page, int size);
    public PageResult<MessageDTO> listGroupHistory(Long userId, Long groupId, int page, int size);
    public void markMessageRead(Long userId, Long messageId);
    public void pushOfflineNotifications(Long userId);
}
```

### 8.2.9 文件服务

```java
public class FileService {
    public FileUploadInitResponse initUpload(Long senderId, FileUploadInitRequest request);
    public void receiveChunk(Long senderId, FileChunkRequest request);
    public FileResumeInfo getResumeInfo(Long senderId, String fileId);
    public void completeUpload(Long senderId, String fileId);
    public void sendFileMessage(Long senderId, Long receiverId, String fileId, ChatTargetType targetType);
}
```

### 8.2.10 心跳处理器

```java
public class ServerHeartbeatHandler extends ChannelInboundHandlerAdapter {
    public void userEventTriggered(ChannelHandlerContext ctx, Object evt);
    public void channelRead(ChannelHandlerContext ctx, Object msg);
}
```

---

## 8.3 client 模块

### 8.3.1 客户端启动类

```java
public class ChatClientApplication {
    public static void main(String[] args);
}
```

### 8.3.2 Netty 客户端

```java
public class NettyChatClient {
    public void connect(String host, int port);
    public void send(MessagePacket<?> packet);
    public void close();
}
```

### 8.3.3 客户端上下文

```java
public class ClientContext {
    private Long currentUserId;
    private String username;
    private boolean loggedIn;
}
```

### 8.3.4 命令处理

```java
public interface ClientCommand {
    String name();
    void execute(Scanner scanner, NettyChatClient client, ClientContext context);
}
```

具体命令：

```text
RegisterCommand
LoginCommand
FriendApplyCommand
FriendListCommand
PrivateChatCommand
GroupCreateCommand
GroupChatCommand
FileSendCommand
HistoryCommand
LogoutCommand
```

---

## 9. 类图，文本版

```text
MessagePacket<T>
    ├── RegisterRequest
    ├── LoginRequest
    ├── PrivateChatRequest
    ├── GroupChatRequest
    ├── FileChunkRequest
    └── HeartbeatPing

NettyChatServer
    ├── ServerChannelInitializer
    ├── JsonPacketDecoder
    ├── JsonPacketEncoder
    ├── ServerHeartbeatHandler
    └── ServerMessageDispatcher

ServerMessageDispatcher
    ├── AccountHandler -> AccountService -> UserMapper
    ├── FriendHandler  -> FriendService  -> FriendMapper
    ├── GroupHandler   -> GroupService   -> GroupMapper
    ├── ChatHandler    -> ChatService    -> MessageMapper
    └── FileHandler    -> FileService    -> FileMapper

OnlineUserManager
    └── ConcurrentHashMap<Long, Channel>
```

---

## 10. 数据库设计

## 10.1 用户表 users

```sql
CREATE TABLE users (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    email VARCHAR(100) UNIQUE,
    phone VARCHAR(30) UNIQUE,
    nickname VARCHAR(50),
    avatar_url VARCHAR(255),
    status VARCHAR(20) NOT NULL DEFAULT 'ACTIVE',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    last_login_at DATETIME NULL
);
```

字段说明：

| 字段 | 说明 |
|---|---|
| id | 用户 ID |
| username | 登录用户名，唯一 |
| password_hash | 加密后的密码 |
| email | 邮箱 |
| phone | 手机号 |
| status | ACTIVE / DELETED / BANNED |

---

## 10.2 验证码表 verify_codes

```sql
CREATE TABLE verify_codes (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    target VARCHAR(100) NOT NULL,
    code VARCHAR(10) NOT NULL,
    scene VARCHAR(30) NOT NULL,
    used TINYINT NOT NULL DEFAULT 0,
    expire_at DATETIME NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

---

## 10.3 好友关系表 friends

```sql
CREATE TABLE friends (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id BIGINT NOT NULL,
    friend_id BIGINT NOT NULL,
    remark VARCHAR(100),
    blocked TINYINT NOT NULL DEFAULT 0,
    status VARCHAR(20) NOT NULL DEFAULT 'ACTIVE',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_user_friend (user_id, friend_id),
    INDEX idx_friend_id (friend_id)
);
```

---

## 10.4 好友申请表 friend_requests

```sql
CREATE TABLE friend_requests (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    from_user_id BIGINT NOT NULL,
    to_user_id BIGINT NOT NULL,
    remark VARCHAR(255),
    status VARCHAR(20) NOT NULL DEFAULT 'PENDING',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    handled_at DATETIME NULL,
    INDEX idx_to_user_status (to_user_id, status)
);
```

---

## 10.5 群组表 groups

```sql
CREATE TABLE chat_groups (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(100) NOT NULL,
    description VARCHAR(255),
    owner_id BIGINT NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'ACTIVE',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

---

## 10.6 群成员表 group_members

```sql
CREATE TABLE group_members (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    group_id BIGINT NOT NULL,
    user_id BIGINT NOT NULL,
    role VARCHAR(20) NOT NULL DEFAULT 'MEMBER',
    status VARCHAR(20) NOT NULL DEFAULT 'ACTIVE',
    joined_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_group_user (group_id, user_id),
    INDEX idx_user_id (user_id)
);
```

---

## 10.7 加群申请表 group_join_requests

```sql
CREATE TABLE group_join_requests (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    group_id BIGINT NOT NULL,
    user_id BIGINT NOT NULL,
    reason VARCHAR(255),
    status VARCHAR(20) NOT NULL DEFAULT 'PENDING',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    handled_by BIGINT NULL,
    handled_at DATETIME NULL,
    INDEX idx_group_status (group_id, status)
);
```

---

## 10.8 消息表 messages

```sql
CREATE TABLE messages (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    message_id VARCHAR(64) NOT NULL UNIQUE,
    sender_id BIGINT NOT NULL,
    target_id BIGINT NOT NULL,
    chat_type VARCHAR(20) NOT NULL,
    message_type VARCHAR(20) NOT NULL,
    content TEXT,
    file_id VARCHAR(64) NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'SENT',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_private_history (chat_type, sender_id, target_id, created_at),
    INDEX idx_group_history (chat_type, target_id, created_at)
);
```

字段说明：

- `chat_type`：PRIVATE / GROUP
- `message_type`：TEXT / FILE / IMAGE，可先实现 TEXT 和 FILE
- `target_id`：私聊时为接收方 user_id，群聊时为 group_id

---

## 10.9 消息接收状态表 message_receipts

```sql
CREATE TABLE message_receipts (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    message_id VARCHAR(64) NOT NULL,
    receiver_id BIGINT NOT NULL,
    read_status VARCHAR(20) NOT NULL DEFAULT 'UNREAD',
    delivered_at DATETIME NULL,
    read_at DATETIME NULL,
    UNIQUE KEY uk_msg_receiver (message_id, receiver_id),
    INDEX idx_receiver_unread (receiver_id, read_status)
);
```

---

## 10.10 文件表 files

```sql
CREATE TABLE files (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    file_id VARCHAR(64) NOT NULL UNIQUE,
    owner_id BIGINT NOT NULL,
    original_name VARCHAR(255) NOT NULL,
    storage_path VARCHAR(500) NOT NULL,
    file_size BIGINT NOT NULL,
    md5 VARCHAR(64),
    status VARCHAR(20) NOT NULL DEFAULT 'UPLOADING',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    completed_at DATETIME NULL
);
```

---

## 10.11 文件分片表 file_chunks

```sql
CREATE TABLE file_chunks (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    file_id VARCHAR(64) NOT NULL,
    chunk_index INT NOT NULL,
    chunk_size INT NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'UPLOADED',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_file_chunk (file_id, chunk_index)
);
```

---

## 11. 通信协议设计

### 11.1 数据帧格式

```text
+----------------+--------------------+
| length 4 bytes | JSON body N bytes  |
+----------------+--------------------+
```

### 11.2 请求示例：登录

```json
{
  "requestId": "uuid",
  "type": "LOGIN_REQ",
  "timestamp": 1710000000000,
  "body": {
    "username": "alice",
    "password": "123456"
  }
}
```

### 11.3 响应示例：登录成功

```json
{
  "requestId": "uuid",
  "type": "LOGIN_RESP",
  "timestamp": 1710000001000,
  "body": {
    "success": true,
    "userId": 1,
    "nickname": "Alice"
  }
}
```

### 11.4 错误响应

```json
{
  "requestId": "uuid",
  "type": "ERROR_RESP",
  "timestamp": 1710000002000,
  "body": {
    "code": "FRIEND_NOT_FOUND",
    "message": "You are not friends, private chat is not allowed."
  }
}
```

---

## 12. 两人分工方案

### 人员设定

- A 同学：服务端主负责人，重点做 Netty、数据库、账号、聊天、日志
- B 同学：客户端主负责人，重点做 CLI、命令交互、好友群组功能、文件传输、文档和测试

实际开发时两人都要懂整体协议，不能只写自己的部分。

---

## 13. 两周详细工期安排

### Day 1：项目启动与环境搭建

A：

- 创建 GitHub 仓库
- 创建 Maven 多模块项目
- 配置 JDK 17、Maven、Log4j2
- 搭建 Netty 服务端空项目

B：

- 克隆仓库
- 搭建 Netty 客户端空项目
- 编写 README 初稿
- 整理需求列表和任务 Issue

共同产出：

- 项目能启动
- GitHub 仓库能 push/pull
- `main`、`dev`、功能分支建立

---

### Day 2：通信协议与基础网络

A：

- 实现 `MessagePacket`
- 实现 JSON 编解码器
- 实现服务端连接管理
- 实现 Echo 测试

B：

- 实现客户端连接服务端
- 实现客户端发送 JSON 消息
- 实现基础菜单框架

共同产出：

- 客户端和服务端可以互发消息
- 解决粘包半包

---

### Day 3：MySQL 与账号注册登录

A：

- 设计并执行用户表 SQL
- 配置 MyBatis 和 HikariCP
- 实现注册、登录 Service
- 实现密码 BCrypt 加密

B：

- 实现注册命令
- 实现登录命令
- 保存客户端登录状态
- 编写账号模块测试用例

共同产出：

- 用户可以注册和登录
- 登录成功后服务端记录在线状态

---

### Day 4：注销、验证码、离线通知框架

A：

- 实现注销和账号状态
- 实现验证码表
- 实现验证码生成与校验
- 搭建离线通知查询框架

B：

- 实现找回密码 CLI 流程
- 实现验证码登录 CLI 流程
- 增加非法输入处理

共同产出：

- 账号模块基本完成
- 提高功能 1：邮箱验证码完成或模拟完成

---

### Day 5：好友申请与好友列表

A：

- 设计好友表和好友申请表
- 实现添加好友、同意、拒绝
- 实现好友在线状态查询

B：

- 实现好友菜单
- 实现发送好友申请
- 实现处理好友申请
- 实现好友列表显示

共同产出：

- 好友申请全流程跑通

---

### Day 6：私聊与历史消息

A：

- 设计消息表和消息接收状态表
- 实现私聊发送
- 实现非好友禁止私聊
- 实现历史消息分页查询

B：

- 实现私聊命令
- 实现收到消息实时显示
- 实现历史消息查看

共同产出：

- 好友之间可以在线私聊
- 离线用户上线后能看到未读提醒

---

### Day 7：群组基础功能

A：

- 设计群表、群成员表、加群申请表
- 实现创建群、查看已加入群、查看群成员
- 实现退出群

B：

- 实现群组菜单
- 实现创建群命令
- 实现查看群列表和成员列表
- 实现退出群命令

共同产出：

- 群组基础管理完成

---

### Day 8：加群审批和群权限

A：

- 实现申请加群
- 实现管理员审批
- 实现添加/删除管理员
- 实现移除群成员
- 实现群主解散群

B：

- 实现加群申请命令
- 实现管理员审批命令
- 实现群管理命令
- 测试权限边界

共同产出：

- 群权限逻辑完成

---

### Day 9：群聊与群历史消息

A：

- 实现群消息发送
- 实现群成员校验
- 实现群消息广播
- 实现群历史消息查询

B：

- 实现群聊命令
- 实现群消息接收显示
- 实现群历史记录查看

共同产出：

- 群内成员可以聊天
- 被移除成员无法发言

---

### Day 10：文件发送基础版

A：

- 设计文件表
- 实现文件上传初始化
- 实现文件保存到服务器本地目录
- 实现文件消息写入数据库

B：

- 实现客户端选择文件
- 实现文件读取与发送
- 实现接收文件通知

共同产出：

- 在线用户之间可以发送文件

---

### Day 11：断点续传与心跳检测

A：

- 实现 `file_chunks` 表
- 实现分片上传记录
- 实现断点续传查询
- 实现服务端 IdleStateHandler 心跳检测

B：

- 实现客户端文件分片发送
- 实现断线后查询已上传分片
- 实现客户端定时发送心跳，但不要额外阻塞主逻辑

共同产出：

- 提高功能 2：断点续传完成
- TCP 心跳检测完成

---

### Day 12：TLS、日志、稳定性优化

A：

- 增加 TLS 配置，可用自签名证书
- 完善服务端日志
- 统一异常处理
- 业务线程池优化

B：

- 完善客户端异常提示
- 优化菜单体验
- 增加参数指定 IP:Port
- 编写用户文档

共同产出：

- 提高功能 3：TLS 通信加密完成或提供可运行开关
- 日志和稳定性明显提升

---

### Day 13：集成测试与 Bug 修复

A：

- 编写核心 Service 单元测试
- 压测多客户端连接
- 修复服务端异常

B：

- 完整跑通用户使用流程
- 整理测试截图
- 补充 README 和用户文档

共同产出：

- 所有 P0 功能验收通过
- 至少 3 个提高功能有演示证据

---

### Day 14：发布、答辩材料、GitHub 整理

A：

- 打包 server jar
- 整理数据库 SQL
- 写部署说明
- 打 tag：`v1.0.0`

B：

- 打包 client jar
- 完成用户手册
- 整理项目演示步骤
- 检查 GitHub 仓库结构

共同产出：

- GitHub 仓库完整
- README 完整
- 项目可从零部署运行

---

## 14. GitHub 第一次使用流程

### 14.1 创建仓库

仓库名建议：

```text
java-chatroom
```

不要勾选太多模板，建议只初始化 README。

### 14.2 本地首次提交

```bash
git clone https://github.com/你的用户名/java-chatroom.git
cd java-chatroom
```

复制项目文件后：

```bash
git add .
git commit -m "chore: init java chatroom project"
git push origin main
```

### 14.3 创建 dev 分支

```bash
git checkout -b dev
git push -u origin dev
```

### 14.4 每个人开发自己的分支

A：

```bash
git checkout dev
git pull
git checkout -b feature/server-netty
```

B：

```bash
git checkout dev
git pull
git checkout -b feature/client-cli
```

### 14.5 每日提交代码

```bash
git status
git add .
git commit -m "feat: add login command"
git push
```

### 14.6 合并代码

推荐用 GitHub Pull Request：

1. 打开 GitHub 仓库
2. 点击 Pull requests
3. New pull request
4. base 选择 `dev`
5. compare 选择自己的功能分支
6. 检查改动
7. 另一个人 Review
8. Merge pull request

### 14.7 发布版本

```bash
git checkout main
git merge dev
git tag v1.0.0
git push origin main
git push origin v1.0.0
```

---

## 15. README 推荐结构

```markdown
# JavaChatRoom

## 1. 项目介绍

基于 Java 17、Netty、MySQL 的 C/S 聊天室系统。

## 2. 功能列表

- 账号注册、登录、注销
- 好友添加、删除、查询、在线状态
- 私聊、群聊、历史消息
- 离线消息通知
- 文件发送与断点续传
- TCP 心跳检测
- 服务端日志
- TLS 加密

## 3. 技术栈

Java 17, Netty, MySQL, MyBatis, Maven, Log4j2, JUnit 5

## 4. 项目结构

粘贴项目目录树。

## 5. 数据库初始化

执行 `docs/database.sql`。

## 6. 启动服务端

java -jar chat-server.jar --host 0.0.0.0 --port 9000

## 7. 启动客户端

java -jar chat-client.jar --host 127.0.0.1 --port 9000

## 8. 演示流程

注册两个用户 -> 添加好友 -> 私聊 -> 创建群 -> 加群 -> 群聊 -> 文件发送。

## 9. 分工说明

A：服务端、数据库、账号、聊天、日志  
B：客户端、好友、群组、文件、文档测试

## 10. 提高功能

- 邮箱验证码
- TLS 通信加密
- 文件断点续传
- 主从 Reactor 模型
```

---

## 16. 推荐验收演示流程

1. 启动 MySQL
2. 执行 `database.sql`
3. 启动服务端
4. 启动客户端 A 和客户端 B
5. A 注册并登录
6. B 注册并登录
7. A 添加 B 为好友
8. B 同意好友申请
9. A 给 B 发送私聊消息
10. B 下线
11. A 给 B 发送离线消息
12. B 上线收到未读通知
13. A 创建群
14. B 申请加群
15. A 批准
16. A 和 B 群聊
17. A 发送文件给 B
18. 展示心跳日志
19. 展示服务器日志
20. 展示 GitHub 仓库和 README

---

## 17. 建议优先级取舍

两周项目不要一开始就做 GUI。建议顺序是：

1. 先完成 CLI 可运行版本
2. 再完成数据库和核心业务
3. 再补提高功能
4. 最后才做 JavaFX GUI

如果时间不够，必须优先保住：

- 账号
- 好友
- 私聊
- 群组
- 群聊
- 历史消息
- 离线通知
- 心跳
- 日志
- MySQL
- GitHub 文档

GUI 可以写在 README 中作为扩展计划，但不要牺牲核心功能。

---

## 18. 最小可运行版本 MVP

MVP 必须包含：

- 注册
- 登录
- 好友申请和同意
- 私聊
- 创建群
- 加群
- 群聊
- 历史消息
- 离线消息提醒
- MySQL 存储
- Netty NIO
- 日志
- 心跳

MVP 完成后，再做：

- 验证码
- 文件传输
- 断点续传
- TLS
- GUI

---

## 19. 风险与解决方案

| 风险 | 解决方案 |
|---|---|
| Netty 学习时间不够 | 先写 Echo，再写自定义协议，再接业务 |
| MySQL 不熟 | 先用简单 SQL，不要过早引入复杂 ORM |
| 功能太多做不完 | 先做 MVP，再做提高功能 |
| 两人代码冲突 | 每天从 dev 拉最新代码，小分支开发 |
| 文件断点续传复杂 | 先做普通文件发送，再做分片记录 |
| TLS 配置困难 | 提供普通模式和 TLS 模式两个启动配置 |
| CLI 交互混乱 | 菜单分层，所有输入统一校验 |

---

## 20. 最终交付物清单

- GitHub 源码仓库
- `README.md`
- `docs/requirement.md`
- `docs/database.sql`
- `docs/api_protocol.md`
- `docs/user_manual.md`
- 服务端 jar 包
- 客户端 jar 包
- 演示截图或录屏
- 测试用例
- 日志文件示例
- 项目答辩说明
