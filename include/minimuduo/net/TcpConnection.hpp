#pragma once

#include "minimuduo/net/Buffer.hpp"
#include "minimuduo/net/Callbacks.hpp"
#include "minimuduo/net/NonCopyable.hpp"

#include <any>
#include <functional>
#include <atomic>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <vector>

namespace minimuduo::net {

class Channel;
class EventLoop;

class TcpConnection final
    : private NonCopyable,
      public std::enable_shared_from_this<TcpConnection> {
public:
    using SendCompleteCallback = std::function<void()>;

    TcpConnection(
        EventLoop* loop,
        std::string name,
        int socketFd,
        const sockaddr_in& localAddress,
        const sockaddr_in& peerAddress);
    ~TcpConnection();

    EventLoop* getLoop() const noexcept;
    const std::string& name() const noexcept;
    bool connected() const noexcept;

    std::string localAddressText() const;
    std::string peerAddressText() const;

    void send(const std::string& message);
    void send(
        const std::string& message,
        SendCompleteCallback completion
    );
    void shutdown();
    void forceClose();

    void setConnectionCallback(ConnectionCallback callback);
    void setMessageCallback(MessageCallback callback);
    void setWriteCompleteCallback(WriteCompleteCallback callback);
    void setCloseCallback(CloseCallback callback);

    void setContext(std::any context);
    const std::any& getContext() const noexcept;
    std::any* getMutableContext() noexcept;

    void connectEstablished();
    void connectDestroyed();

private:
    enum class State {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting,
    };

    void setState(State state) noexcept;
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(
        std::string message,
        SendCompleteCallback completion
    );
    void shutdownInLoop();
    void forceCloseInLoop();

    EventLoop* loop_;
    const std::string name_;
    std::atomic<State> state_;
    int socketFd_;
    std::unique_ptr<Channel> channel_;
    const sockaddr_in localAddress_;
    const sockaddr_in peerAddress_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
    std::vector<SendCompleteCallback> pendingSendCompletions_;
    std::any context_;
};

}  // namespace minimuduo::net
