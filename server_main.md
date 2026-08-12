#### server_main

1.   加载配置`load_mysql_config`
2.   创建数据库 `MySqlDatabase database`;
3.   连接数据库
4.   创建主reactor————main_loop
5.   创建业务层chatserver
6.   创建tcpserver
7.   调用tcpserver的start
8.   调用mainloop的loop开始epoll_wait
9.   main_loop.loop();

```text
main()
 ├─ 解析参数,加载配置
 ├─ load_mysql_config() + database.connect()
 ├─ load_redis_config() + redis.connect() + redis.ping()
 ├─ 构造 listen_address
 ├─ 创建 EventLoop main_loop
 ├─ 创建 TcpServer tcp_server(&main_loop, listen_address, "chat")
 ├─ tcp_server.setThreadNum(worker_threads)      // 启动 worker_threads 个子循环
 ├─ chat_server.emplace( tcp_server, database, redis, server_instance_id, ... )
 │   └─ 此处会调用 tcp_server.setConnectionCallback(...) 等，绑定业务处理函数
 ├─ tcp_server.start()
 │   ├─ 开启 listen fd
 │   ├─ 启动 SubReactor 线程（每个线程运行 EventLoop::loop()）
 │   └─ 主线程继续往下
 ├─ main_loop.loop()
 │   ├─ 注册 Acceptor 到 main_loop
 │   ├─ 开始 epoll_wait 循环
 │   ├─ 接受新连接，通过 round-robin 分配给某个 SubReactor
 │   └─ 直到程序终止
 └─ return 0
```

#### sqlite

```
【客户端启动】
    ↓
1. open() 
    ├── sqlite3_open_v2()  → 打开物理文件
    ├── sqlite3_busy_timeout() → 设置等待超时
    └── initialize_schema()
            ├── sqlite3_exec(PRAGMA...) → WAL模式调优
            └── sqlite3_exec(CREATE TABLE) → 建表
    ↓
【运行时业务】
    ↓
2. 写入 (cache_*)
    ├── sqlite3_prepare_v2(SQL带?) → 准备
    ├── sqlite3_bind_*(...) → 绑值
    ├── sqlite3_step() → 执行 (检查 SQLITE_DONE)
    └── 自动 finalize (RAII析构)
    ↓
3. 读取 (recent_*)
    ├── sqlite3_prepare_v2 → 准备
    ├── sqlite3_bind_* → 绑值
    ├── while(sqlite3_step() == SQLITE_ROW)
    │   └── sqlite3_column_* → 取字段
    ├── std::reverse() → 调正顺序
    └── 自动 finalize (RAII析构)
    ↓
【程序退出】
    ↓
4. ~SqliteClient()
    └── sqlite3_close(database_) → 关闭连接
```



#### tcp_server.start()

1.   threadPool_->start();————创建指定数量的线程，并且添加编号

2.   acceptor_->listen();——`acceptChannel_->enableReading();`——

     1.    events_ |= kReadEvent;增加监听选项，但是还要把这个选项送到linux内核中

     2.  update();

         ```c
         Channel::update() 
            → 调用 eventLoop_->updateChannel(this)   // 传给事件循环
                → 调用 poller_->updateChannel(this)  // 传给 IO 复用器（如 Epoll）
                    → 调用 ::epoll_ctl(EPOLL_CTL_ADD/MOD, ...) // 最终系统调用
         ```

     ​	

     最终成功把主线程里面的兴趣事件添加了监听可读事件

     #### main_loop.loop();

     1.   poller_->poll——epollwait阻塞等待——fillActiveChannels向activeChannels填充channel

     2.   channel->handleEvent();——交给channel——判断是否需要保护tied,不需要是acceptor和eventloop,直接调用 handleEventWithGuard();

          但是如果需要的花，也就是客户端连接，那么先获得sharedptr再 handleEventWithGuard();———调用不同的回调函数

     3.   doPendingFunctors();