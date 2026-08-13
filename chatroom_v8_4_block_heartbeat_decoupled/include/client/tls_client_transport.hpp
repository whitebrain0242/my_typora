#pragma once

#include "config.hpp"
#include "minimuduo/net/TlsContext.hpp"

#include <cstddef>
#include <string>

enum class TransportReadStatus {
    Data,
    Closed,
    Retry,
    Error
};

struct TransportReadResult {
    TransportReadStatus status =
        TransportReadStatus::Error;

    std::size_t bytes = 0;
    std::string error;
};

class TlsClientTransport {
public:
    TlsClientTransport() = default;
    ~TlsClientTransport();

    TlsClientTransport(
        const TlsClientTransport&
    ) = delete;

    TlsClientTransport& operator=(
        const TlsClientTransport&
    ) = delete;

    bool connect(
        const std::string& ip,
        int port,
        const TlsClientConfig& config,
        std::string& error
    );

    bool send(
        const std::string& data,
        std::string& error
    );

    TransportReadResult receive(
        char* buffer,
        std::size_t capacity
    );

    int fd() const noexcept;

    int pending() const noexcept;

    std::string tls_version() const;
    std::string cipher_name() const;

    const std::string&
    peer_identity() const noexcept;

    bool connected() const noexcept;

    void shutdown();

private:
    minimuduo::net::TlsClientContext tls_context_;
    minimuduo::net::SslPtr ssl_;

    int socket_fd_ = -1;

    std::string peer_identity_;
    bool connected_ = false;
};
