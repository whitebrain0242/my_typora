#include "chat_server.hpp"
#include "config.hpp"
#include "protocol.hpp"

#include "integration/redis_client.hpp"

#include "minimuduo/net/EventLoop.hpp"
#include "minimuduo/net/TcpServer.hpp"

#include <arpa/inet.h>
#include <csignal>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <string>

int main(int argc, char* argv[]) {
    int port = 9000;
    std::string mysql_config_path = "config/mysql.conf";
    std::string redis_config_path = "config/redis.conf";
    int worker_threads = 4;
    std::filesystem::path file_storage_root = "data/server_files";

    if (argc >= 2 &&
        !parse_port(argv[1], port)) {
        std::cerr << "invalid server port\n";
        return 1;
    }

    if (argc >= 3) {
        mysql_config_path = argv[2];
    }

    // Backward compatible with v8.0:
    //   chat_server 9000 mysql.conf 8
    // New form:
    //   chat_server 9000 mysql.conf redis.conf 8
    if (argc >= 4) {
        std::size_t parsed = 0;

        if (parse_count(
                argv[3],
                1U,
                64U,
                parsed
            )) {
            worker_threads =
                static_cast<int>(parsed);
        } else {
            redis_config_path = argv[3];
        }
    }

    if (argc >= 5) {
        std::size_t parsed = 0;

        if (!parse_count(
                argv[4],
                1U,
                64U,
                parsed
            )) {
            std::cerr
                << "worker thread count must be 1-64\n";
            return 1;
        }

        worker_threads =
            static_cast<int>(parsed);
    }

    if (argc >= 6) {
        file_storage_root = argv[5];
    }

    (void)std::signal(SIGPIPE, SIG_IGN);

    std::string error;

    MySqlConfig mysql_config;
    if (!load_mysql_config(
            mysql_config_path,
            mysql_config,
            error
        )) {
        std::cerr << error << '\n';
        return 1;
    }

    RedisConfig redis_config;
    if (!load_redis_config(
            redis_config_path,
            redis_config,
            error
        )) {
        std::cerr << error << '\n';
        return 1;
    }

    MySqlDatabase database;
    if (!database.connect(
            mysql_config,
            error
        )) {
        std::cerr
            << "MySQL connection failed: "
            << error
            << '\n';
        return 1;
    }

    RedisClient redis;
    if (!redis.connect(
            redis_config,
            error
        ) ||
        !redis.ping(error)) {
        std::cerr
            << "Redis connection failed: "
            << error
            << '\n';
        return 1;
    }

    sockaddr_in listen_address{};
    listen_address.sin_family = AF_INET;
    listen_address.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_address.sin_port = htons(
        static_cast<std::uint16_t>(port)
    );

    minimuduo::net::EventLoop main_loop;

    std::optional<ChatServer> chat_server;

    minimuduo::net::TcpServer tcp_server(
        &main_loop,
        listen_address,
        "chat"
    );

    tcp_server.setThreadNum(worker_threads);

    const std::string server_instance_id =
        redis_config.server_name +
        ":" +
        std::to_string(port);

    try {
        chat_server.emplace(
            tcp_server,
            database,
            redis,
            server_instance_id,
            redis_config.presence_ttl_seconds,
            file_storage_root
        );
    } catch (const std::exception& exception) {
        std::cerr
            << "ChatServer initialization failed: "
            << exception.what()
            << '\n';
        return 1;
    }

    tcp_server.start();

    std::cout
        << "chat_server v8.2 listening on port "
        << port
        << " with 1 MainReactor + "
        << worker_threads
        << " SubReactor(s)\n"
        << "Redis presence server id: "
        << server_instance_id
        << '\n'
        << "File storage root: "
        << file_storage_root.string()
        << '\n';

    main_loop.loop();
    return 0;
}
