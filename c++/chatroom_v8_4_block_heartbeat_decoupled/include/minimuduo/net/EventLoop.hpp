#pragma once

#include "minimuduo/net/NonCopyable.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace minimuduo::net {

class Channel;
class Poller;

class EventLoop final : private NonCopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    void runInLoop(Functor callback);
    void queueInLoop(Functor callback);

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

    bool isInLoopThread() const noexcept;
    void assertInLoopThread() const;

private:
    void wakeup();
    void handleWakeupRead();
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    std::atomic<bool> callingPendingFunctors_;

    const std::thread::id threadId_;

    std::unique_ptr<Poller> poller_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    ChannelList activeChannels_;

    std::mutex mutex_;
    std::vector<Functor> pendingFunctors_;
};

}  // namespace minimuduo::net
