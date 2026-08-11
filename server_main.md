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