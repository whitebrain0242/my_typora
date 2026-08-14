#include "server/heartbeat_manager.hpp"

#include "minimuduo/net/TcpConnection.hpp"

#include <iostream>
#include <utility>
#include <vector>

HeartbeatManager::HeartbeatManager(
    HeartbeatConfig config
)
    : config_(std::move(config)) {}

HeartbeatManager::~HeartbeatManager() {
    stop();
}

void HeartbeatManager::start() {
    if (thread_.joinable()) {
        return;
    }

    stopping_.store(false);

    thread_ =
        std::thread(
            [this] {
                loop();
            }
        );
}

void HeartbeatManager::stop() {
    stopping_.store(true);
    wait_cv_.notify_all();

    if (thread_.joinable()) {
        thread_.join();
    }

    std::lock_guard<std::mutex> lock(
        mutex_
    );

    entries_.clear();
}

void HeartbeatManager::attach(
    const TcpConnectionPtr& connection
) {
    if (connection == nullptr) {
        return;
    }

    const Clock::time_point now =
        Clock::now();

    std::lock_guard<std::mutex> lock(
        mutex_
    );

    Entry entry;
    entry.connection =
        connection;
    entry.last_activity =
        now;
    entry.last_ping =
        now;

    entries_[
        connection->name()
    ] = std::move(entry);
}

void HeartbeatManager::detach(
    const TcpConnectionPtr& connection
) {
    if (connection == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(
        mutex_
    );

    entries_.erase(
        connection->name()
    );
}

void HeartbeatManager::note_activity(
    const TcpConnectionPtr& connection
) {
    if (connection == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(
        mutex_
    );

    const auto iterator =
        entries_.find(
            connection->name()
        );

    if (iterator ==
        entries_.end()) {
        return;
    }

    iterator->second.last_activity =
        Clock::now();
}

bool HeartbeatManager::note_pong(
    const TcpConnectionPtr& connection,
    std::uint64_t nonce
) {
    if (connection == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(
        mutex_
    );

    const auto iterator =
        entries_.find(
            connection->name()
        );

    if (iterator ==
        entries_.end()) {
        return false;
    }

    iterator->second.last_activity =
        Clock::now();

    return
        nonce ==
        iterator->second.last_nonce;
}

void HeartbeatManager::loop() {
    std::unique_lock<std::mutex> wait_lock(
        wait_mutex_
    );

    while (!stopping_.load()) {
        const bool stopping =
            wait_cv_.wait_for(
                wait_lock,
                config_.scan_interval,
                [this] {
                    return
                        stopping_.load();
                }
            );

        if (stopping) {
            break;
        }

        wait_lock.unlock();
        scan_once();
        wait_lock.lock();
    }
}

void HeartbeatManager::scan_once() {
    const Clock::time_point now =
        Clock::now();

    std::vector<PingAction>
        pings;

    std::vector<TcpConnectionPtr>
        timed_out;

    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );

        for (auto iterator =
                 entries_.begin();
             iterator !=
                 entries_.end();) {
            TcpConnectionPtr connection =
                iterator->second
                    .connection.lock();

            if (connection == nullptr ||
                !connection->connected()) {
                iterator =
                    entries_.erase(
                        iterator
                    );
                continue;
            }

            Entry& entry =
                iterator->second;

            if (now -
                    entry.last_activity >=
                config_.timeout) {
                timed_out.push_back(
                    connection
                );

                iterator =
                    entries_.erase(
                        iterator
                    );

                continue;
            }

            if (now -
                    entry.last_ping >=
                config_.ping_interval) {
                entry.last_nonce =
                    next_nonce_++;

                entry.last_ping =
                    now;

                pings.push_back(
                    {
                        connection,
                        entry.last_nonce
                    }
                );
            }

            ++iterator;
        }
    }

    for (const PingAction& action :
         pings) {
        action.connection->send(
            "PING " +
            std::to_string(
                action.nonce
            ) +
            "\n"
        );
    }

    for (const TcpConnectionPtr& connection :
         timed_out) {
        std::cerr
            << "heartbeat timeout: "
            << connection->name()
            << " from "
            << connection->peerAddressText()
            << '\n';

        connection->send(
            "[system] heartbeat timeout; "
            "connection will close.\n"
        );

        connection->forceClose();
    }
}
