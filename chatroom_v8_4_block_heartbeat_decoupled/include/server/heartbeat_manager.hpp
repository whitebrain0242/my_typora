#pragma once

#include "minimuduo/net/Callbacks.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

struct HeartbeatConfig {
    std::chrono::milliseconds ping_interval{
        20'000
    };

    std::chrono::milliseconds timeout{
        60'000
    };

    std::chrono::milliseconds scan_interval{
        1'000
    };
};

class HeartbeatManager {
public:
    using TcpConnectionPtr =
        minimuduo::net::TcpConnectionPtr;

    explicit HeartbeatManager(
        HeartbeatConfig config = {}
    );

    ~HeartbeatManager();

    HeartbeatManager(
        const HeartbeatManager&
    ) = delete;

    HeartbeatManager& operator=(
        const HeartbeatManager&
    ) = delete;

    void start();
    void stop();

    void attach(
        const TcpConnectionPtr& connection
    );

    void detach(
        const TcpConnectionPtr& connection
    );

    void note_activity(
        const TcpConnectionPtr& connection
    );

    bool note_pong(
        const TcpConnectionPtr& connection,
        std::uint64_t nonce
    );

private:
    using Clock =
        std::chrono::steady_clock;

    struct Entry {
        std::weak_ptr<
            minimuduo::net::TcpConnection
        > connection;

        Clock::time_point last_activity;
        Clock::time_point last_ping;
        std::uint64_t last_nonce = 0;
    };

    struct PingAction {
        TcpConnectionPtr connection;
        std::uint64_t nonce = 0;
    };

    void loop();
    void scan_once();

    HeartbeatConfig config_;

    std::atomic<bool> stopping_{
        false
    };

    std::mutex mutex_;
    std::unordered_map<
        std::string,
        Entry
    > entries_;

    std::mutex wait_mutex_;
    std::condition_variable wait_cv_;
    std::thread thread_;

    std::uint64_t next_nonce_ = 1;
};
