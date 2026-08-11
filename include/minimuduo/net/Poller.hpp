#pragma once

#include "minimuduo/net/NonCopyable.hpp"

#include <chrono>
#include <memory>
#include <vector>

struct epoll_event;

namespace minimuduo::net {

class Channel;
class EventLoop;

class Poller final : private NonCopyable {
public:
    using ChannelList = std::vector<Channel*>;

    explicit Poller(EventLoop* loop);
    ~Poller();

    void poll(std::chrono::milliseconds timeout, ChannelList* activeChannels);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    void fillActiveChannels(int eventCount, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    static constexpr int kNew = -1;
    static constexpr int kAdded = 1;
    static constexpr int kDeleted = 2;

    EventLoop* ownerLoop_;
    int epollFd_;
    std::vector<epoll_event> events_;
};

}  // namespace minimuduo::net
