#pragma once

#include "minimuduo/net/NonCopyable.hpp"

#include <memory>
#include <string>
#include <vector>

namespace minimuduo::net {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool final : private NonCopyable {
public:
    EventLoopThreadPool(EventLoop* baseLoop, std::string name);
    ~EventLoopThreadPool();

    void setThreadNum(int threadCount);
    void start();

    EventLoop* getNextLoop();
    const std::vector<EventLoop*>& getAllLoops() const noexcept;

private:
    EventLoop* baseLoop_;
    std::string name_;
    bool started_;
    int threadCount_;
    std::size_t next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};

}  // namespace minimuduo::net
