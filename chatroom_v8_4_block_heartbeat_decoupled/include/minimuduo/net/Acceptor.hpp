#pragma once

#include "minimuduo/net/NonCopyable.hpp"

#include <functional>
#include <memory>
#include <netinet/in.h>

namespace minimuduo::net {

class Channel;
class EventLoop;

class Acceptor final : private NonCopyable {
public:
    using NewConnectionCallback =
        std::function<void(int socketFd, const sockaddr_in& peerAddress)>;

    Acceptor(EventLoop* loop, const sockaddr_in& listenAddress);
    ~Acceptor();

    void setNewConnectionCallback(NewConnectionCallback callback);
    void listen();
    bool listening() const noexcept;

private:
    void handleRead();

    EventLoop* loop_;
    int acceptSocket_;
    std::unique_ptr<Channel> acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
};

}  // namespace minimuduo::net
