#pragma once

#include "minimuduo/net/NonCopyable.hpp"

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace minimuduo::net {

class EventLoop;

class EventLoopThread final : private NonCopyable {
public:
    explicit EventLoopThread(std::string name);
    ~EventLoopThread();

    EventLoop* startLoop();

private:
    void threadFunction();

    EventLoop* loop_;
    bool exiting_;
    std::string name_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

}  // namespace minimuduo::net
