#pragma once

#include "minimuduo/net/Callbacks.hpp"
#include "minimuduo/net/NonCopyable.hpp"

#include <atomic>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <string>

namespace minimuduo::net {

class Acceptor;
class EventLoop;
class EventLoopThreadPool;
class TlsServerContext;

class TcpServer final : private NonCopyable {
public:
    TcpServer(
        EventLoop* loop,
        const sockaddr_in& listenAddress,
        std::string name);
    ~TcpServer();

    void setThreadNum(int threadCount);
    void setTlsContext(
        std::shared_ptr<TlsServerContext> tlsContext
    );
    void setConnectionCallback(ConnectionCallback callback);
    void setMessageCallback(MessageCallback callback);
    void setWriteCompleteCallback(WriteCompleteCallback callback);

    void start();

private:
    void newConnection(int socketFd, const sockaddr_in& peerAddress);
    void removeConnection(const TcpConnectionPtr& connection);
    void removeConnectionInLoop(const TcpConnectionPtr& connection);

    EventLoop* loop_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<EventLoopThreadPool> threadPool_;
    std::shared_ptr<TlsServerContext> tlsContext_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    std::atomic<bool> started_;
    int nextConnectionId_;
    std::map<std::string, TcpConnectionPtr> connections_;
};

}  // namespace minimuduo::net
