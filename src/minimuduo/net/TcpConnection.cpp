#include "minimuduo/net/TcpConnection.hpp"

#include "minimuduo/net/Channel.hpp"
#include "minimuduo/net/EventLoop.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace minimuduo::net {

namespace {

std::string addressToText(const sockaddr_in& address) {
    char ip[INET_ADDRSTRLEN]{};
    if (::inet_ntop(AF_INET, &address.sin_addr, ip, sizeof(ip)) == nullptr) {
        return "unknown";
    }
    return std::string(ip) + ":" + std::to_string(ntohs(address.sin_port));
}

}  // namespace

TcpConnection::TcpConnection(
    EventLoop* loop,
    std::string name,
    int socketFd,
    const sockaddr_in& localAddress,
    const sockaddr_in& peerAddress)
    : loop_(loop),
      name_(std::move(name)),
      state_(State::kConnecting),
      socketFd_(socketFd),
      channel_(std::make_unique<Channel>(loop, socketFd)),
      localAddress_(localAddress),
      peerAddress_(peerAddress) {
    channel_->setReadCallback([this] { handleRead(); });
    channel_->setWriteCallback([this] { handleWrite(); });
    channel_->setCloseCallback([this] { handleClose(); });
    channel_->setErrorCallback([this] { handleError(); });
}

TcpConnection::~TcpConnection() {
    ::close(socketFd_);
}

EventLoop* TcpConnection::getLoop() const noexcept {
    return loop_;
}

const std::string& TcpConnection::name() const noexcept {
    return name_;
}

bool TcpConnection::connected() const noexcept {
    return state_.load() == State::kConnected;
}

std::string TcpConnection::localAddressText() const {
    return addressToText(localAddress_);
}

std::string TcpConnection::peerAddressText() const {
    return addressToText(peerAddress_);
}

void TcpConnection::send(const std::string& message) {
    send(message, {});
}

void TcpConnection::send(
    const std::string& message,
    SendCompleteCallback completion
) {
    if (state_.load() != State::kConnected) {
        return;
    }

    if (loop_->isInLoopThread()) {
        sendInLoop(
            message,
            std::move(completion)
        );
    } else {
        const std::shared_ptr<TcpConnection> self =
            shared_from_this();

        loop_->runInLoop(
            [
                self,
                message,
                completion = std::move(completion)
            ]() mutable {
                self->sendInLoop(
                    message,
                    std::move(completion)
                );
            }
        );
    }
}

void TcpConnection::shutdown() {
    State expected = State::kConnected;
    if (state_.compare_exchange_strong(expected, State::kDisconnecting)) {
        const std::shared_ptr<TcpConnection> self = shared_from_this();
        loop_->runInLoop([self] { self->shutdownInLoop(); });
    }
}

void TcpConnection::forceClose() {
    const State state = state_.load();
    if (state == State::kConnected || state == State::kDisconnecting) {
        setState(State::kDisconnecting);
        const std::shared_ptr<TcpConnection> self = shared_from_this();
        loop_->queueInLoop([self] { self->forceCloseInLoop(); });
    }
}

void TcpConnection::setConnectionCallback(ConnectionCallback callback) {
    connectionCallback_ = std::move(callback);
}

void TcpConnection::setMessageCallback(MessageCallback callback) {
    messageCallback_ = std::move(callback);
}

void TcpConnection::setWriteCompleteCallback(WriteCompleteCallback callback) {
    writeCompleteCallback_ = std::move(callback);
}

void TcpConnection::setCloseCallback(CloseCallback callback) {
    closeCallback_ = std::move(callback);
}

void TcpConnection::setContext(std::any context) {
    context_ = std::move(context);
}

const std::any& TcpConnection::getContext() const noexcept {
    return context_;
}

std::any* TcpConnection::getMutableContext() noexcept {
    return &context_;
}

void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    setState(State::kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    if (connectionCallback_) {
        connectionCallback_(shared_from_this());
    }
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();

    if (state_.load() == State::kConnected) {
        setState(State::kDisconnected);
        channel_->disableAll();
        if (connectionCallback_) {
            connectionCallback_(shared_from_this());
        }
    }

    channel_->remove();
}

void TcpConnection::setState(State state) noexcept {
    state_.store(state);
}

void TcpConnection::handleRead() {
    loop_->assertInLoopThread();

    int savedErrno = 0;
    const ssize_t bytesRead = inputBuffer_.readFd(socketFd_, &savedErrno);

    if (bytesRead > 0) {
        if (messageCallback_) {
            messageCallback_(shared_from_this(), &inputBuffer_);
        }
        return;
    }

    if (bytesRead == 0) {
        handleClose();
        return;
    }

    errno = savedErrno;
    if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        handleError();
        handleClose();
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();

    if (!channel_->isWriting()) {
        return;
    }

    int savedErrno = 0;
    const ssize_t bytesWritten = outputBuffer_.writeFd(socketFd_, &savedErrno);

    if (bytesWritten > 0) {
        outputBuffer_.retrieve(static_cast<std::size_t>(bytesWritten));

        if (outputBuffer_.readableBytes() == 0) {
            channel_->disableWriting();

            if (writeCompleteCallback_) {
                const std::shared_ptr<TcpConnection> self =
                    shared_from_this();

                loop_->queueInLoop(
                    [self, callback = writeCompleteCallback_] {
                        callback(self);
                    }
                );
            }

            std::vector<SendCompleteCallback> completions;
            completions.swap(pendingSendCompletions_);

            for (SendCompleteCallback& completion : completions) {
                if (completion) {
                    loop_->queueInLoop(
                        std::move(completion)
                    );
                }
            }

            if (state_.load() == State::kDisconnecting) {
                shutdownInLoop();
            }
        }
        return;
    }

    errno = savedErrno;
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
        handleError();
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();

    const State state = state_.load();
    if (state == State::kDisconnected) {
        return;
    }

    setState(State::kDisconnected);
    channel_->disableAll();

    const std::shared_ptr<TcpConnection> guard = shared_from_this();
    if (connectionCallback_) {
        connectionCallback_(guard);
    }
    if (closeCallback_) {
        closeCallback_(guard);
    }
}

void TcpConnection::handleError() {
    int socketError = 0;
    socklen_t length = sizeof(socketError);
    if (::getsockopt(
            socketFd_,
            SOL_SOCKET,
            SO_ERROR,
            &socketError,
            &length) < 0) {
        socketError = errno;
    }

    (void)socketError;
}

void TcpConnection::sendInLoop(
    std::string message,
    SendCompleteCallback completion
) {
    loop_->assertInLoopThread();

    if (state_.load() == State::kDisconnected) {
        return;
    }

    ssize_t bytesWritten = 0;
    std::size_t remaining = message.size();
    bool faultError = false;

    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        bytesWritten = ::send(
            socketFd_,
            message.data(),
            message.size(),
            MSG_NOSIGNAL);

        if (bytesWritten >= 0) {
            remaining = message.size() - static_cast<std::size_t>(bytesWritten);
            if (remaining == 0U) {
                if (writeCompleteCallback_) {
                    const std::shared_ptr<TcpConnection> self =
                        shared_from_this();

                    loop_->queueInLoop(
                        [self, callback = writeCompleteCallback_] {
                            callback(self);
                        }
                    );
                }

                if (completion) {
                    loop_->queueInLoop(
                        std::move(completion)
                    );
                }
            }
        } else {
            bytesWritten = 0;
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    if (!faultError && remaining > 0U) {
        outputBuffer_.append(
            message.data() +
                static_cast<std::size_t>(bytesWritten),
            remaining
        );

        if (completion) {
            pendingSendCompletions_.push_back(
                std::move(completion)
            );
        }

        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) {
        (void)::shutdown(socketFd_, SHUT_WR);
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if (state_.load() == State::kConnected ||
        state_.load() == State::kDisconnecting) {
        handleClose();
    }
}

}  // namespace minimuduo::net
