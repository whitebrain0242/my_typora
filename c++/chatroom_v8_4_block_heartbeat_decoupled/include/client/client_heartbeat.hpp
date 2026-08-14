#pragma once

#include <chrono>
#include <string>

class TlsClientTransport;

class ClientHeartbeat {
public:
    using Clock =
        std::chrono::steady_clock;

    explicit ClientHeartbeat(
        std::chrono::milliseconds timeout =
            std::chrono::milliseconds(
                75'000
            )
    );

    void note_server_activity();

    bool consume_protocol_line(
        const std::string& line,
        TlsClientTransport& transport
    );

    bool timed_out() const;

private:
    std::chrono::milliseconds timeout_;
    Clock::time_point last_server_activity_;
};
