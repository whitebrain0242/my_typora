#include "server/heartbeat_manager.hpp"

#include "minimuduo/net/Buffer.hpp"
#include "minimuduo/net/EventLoop.hpp"
#include "minimuduo/net/TcpConnection.hpp"
#include "minimuduo/net/TcpServer.hpp"

#include <arpa/inet.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

bool parse_pong(
    const std::string& line,
    std::uint64_t& nonce
) {
    const std::string prefix =
        "PONG ";

    if (line.rfind(prefix, 0) != 0) {
        return false;
    }

    try {
        nonce =
            static_cast<std::uint64_t>(
                std::stoull(
                    line.substr(
                        prefix.size()
                    )
                )
            );

        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace

int main() {
    constexpr unsigned short port =
        19884;

    minimuduo::net::EventLoop
        main_loop;

    sockaddr_in address{};
    address.sin_family =
        AF_INET;

    address.sin_addr.s_addr =
        htonl(
            INADDR_LOOPBACK
        );

    address.sin_port =
        htons(port);

    minimuduo::net::TcpServer server(
        &main_loop,
        address,
        "heartbeat"
    );

    server.setThreadNum(1);

    HeartbeatConfig config;
    config.ping_interval =
        std::chrono::milliseconds(
            80
        );

    config.timeout =
        std::chrono::milliseconds(
            280
        );

    config.scan_interval =
        std::chrono::milliseconds(
            20
        );

    HeartbeatManager heartbeat(
        config
    );

    heartbeat.start();

    server.setConnectionCallback(
        [&](const minimuduo::net::
                TcpConnectionPtr& connection) {
            if (connection->connected()) {
                heartbeat.attach(
                    connection
                );
            } else {
                heartbeat.detach(
                    connection
                );
            }
        }
    );

    server.setMessageCallback(
        [&](const minimuduo::net::
                TcpConnectionPtr& connection,
            minimuduo::net::Buffer* buffer) {
            heartbeat.note_activity(
                connection
            );

            while (const char* eol =
                       buffer->findEOL()) {
                const std::size_t count =
                    static_cast<std::size_t>(
                        eol -
                        buffer->peek()
                    );

                std::string line =
                    buffer->retrieveAsString(
                        count
                    );

                buffer->retrieve(1U);

                std::uint64_t nonce = 0;

                if (parse_pong(
                        line,
                        nonce
                    )) {
                    (void)heartbeat.note_pong(
                        connection,
                        nonce
                    );
                }
            }
        }
    );

    server.start();

    std::atomic<bool>
        saw_ping{false};

    std::atomic<bool>
        saw_timeout_close{false};

    std::thread client(
        [&] {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(
                    100
                )
            );

            const int fd =
                ::socket(
                    AF_INET,
                    SOCK_STREAM,
                    0
                );

            if (fd < 0) {
                main_loop.quit();
                return;
            }

            if (::connect(
                    fd,
                    reinterpret_cast<
                        const sockaddr*
                    >(&address),
                    sizeof(address)
                ) != 0) {
                ::close(fd);
                main_loop.quit();
                return;
            }

            std::string input;
            char buffer[512]{};
            int pong_count = 0;

            while (true) {
                const ssize_t received =
                    ::recv(
                        fd,
                        buffer,
                        sizeof(buffer),
                        0
                    );

                if (received == 0) {
                    saw_timeout_close.store(
                        true
                    );

                    break;
                }

                if (received < 0) {
                    break;
                }

                input.append(
                    buffer,
                    static_cast<std::size_t>(
                        received
                    )
                );

                while (true) {
                    const std::size_t newline =
                        input.find('\n');

                    if (newline ==
                        std::string::npos) {
                        break;
                    }

                    const std::string line =
                        input.substr(
                            0,
                            newline
                        );

                    input.erase(
                        0,
                        newline + 1U
                    );

                    if (line.rfind(
                            "PING ",
                            0
                        ) != 0) {
                        continue;
                    }

                    saw_ping.store(
                        true
                    );

                    if (pong_count < 2) {
                        const std::string pong =
                            "PONG " +
                            line.substr(5) +
                            "\n";

                        (void)::send(
                            fd,
                            pong.data(),
                            pong.size(),
                            MSG_NOSIGNAL
                        );

                        ++pong_count;
                    }

                    // After two valid replies the test intentionally
                    // becomes silent. The server must close it.
                }
            }

            ::close(fd);
            main_loop.quit();
        }
    );

    main_loop.loop();

    client.join();
    heartbeat.stop();

    return
        saw_ping.load() &&
        saw_timeout_close.load()
            ? EXIT_SUCCESS
            : EXIT_FAILURE;
}
