#include "minimuduo/net/Channel.hpp"

#include "minimuduo/net/EventLoop.hpp"

#include <cassert>
#include <sys/epoll.h>
#include <utility>

namespace minimuduo::net {

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(kNoneEvent),
      revents_(0),
      index_(-1),
      tied_(false) {}

Channel::~Channel() = default;

void Channel::handleEvent() {
    if (!tied_) {
        handleEventWithGuard();
        return;
    }

    const std::shared_ptr<void> guard = tie_.lock();
    if (guard) {
        handleEventWithGuard();
    }
}

void Channel::setReadCallback(EventCallback callback) {
    readCallback_ = std::move(callback);
}

void Channel::setWriteCallback(EventCallback callback) {
    writeCallback_ = std::move(callback);
}

void Channel::setCloseCallback(EventCallback callback) {
    closeCallback_ = std::move(callback);
}

void Channel::setErrorCallback(EventCallback callback) {
    errorCallback_ = std::move(callback);
}

void Channel::tie(const std::shared_ptr<void>& owner) {
    tie_ = owner;
    tied_ = true;
}

int Channel::fd() const noexcept {
    return fd_;
}

std::uint32_t Channel::events() const noexcept {
    return events_;
}

void Channel::setRevents(std::uint32_t revents) noexcept {
    revents_ = revents;
}

bool Channel::isNoneEvent() const noexcept {
    return events_ == kNoneEvent;
}

bool Channel::isWriting() const noexcept {
    return (events_ & kWriteEvent) != 0;
}

void Channel::enableReading() {
    events_ |= kReadEvent;
    update();
}

void Channel::disableReading() {
    events_ &= ~kReadEvent;
    update();
}

void Channel::enableWriting() {
    events_ |= kWriteEvent;
    update();
}

void Channel::disableWriting() {
    events_ &= ~kWriteEvent;
    update();
}

void Channel::disableAll() {
    events_ = kNoneEvent;
    update();
}

void Channel::remove() {
    assert(isNoneEvent());
    loop_->removeChannel(this);
}

int Channel::index() const noexcept {
    return index_;
}

void Channel::setIndex(int index) noexcept {
    index_ = index;
}

EventLoop* Channel::ownerLoop() noexcept {
    return loop_;
}

void Channel::update() {
    loop_->updateChannel(this);
}

void Channel::handleEventWithGuard() {
    if ((revents_ & EPOLLHUP) != 0 && (revents_ & EPOLLIN) == 0) {
        if (closeCallback_) {
            closeCallback_();
        }
        return;
    }

    if ((revents_ & EPOLLERR) != 0) {
        if (errorCallback_) {
            errorCallback_();
        }
    }

    if ((revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) != 0) {
        if (readCallback_) {
            readCallback_();
        }
    }

    if ((revents_ & EPOLLOUT) != 0) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
}

}  // namespace minimuduo::net
