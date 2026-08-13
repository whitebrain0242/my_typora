#include "minimuduo/net/SocketOptions.hpp"

#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

namespace minimuduo::net {

namespace {

bool setIntOption(
    int socketFd,
    int level,
    int option,
    int value,
    const char* name,
    std::string& error
) {
    if (::setsockopt(
            socketFd,
            level,
            option,
            &value,
            sizeof(value)
        ) != 0) {
        error =
            std::string(name) +
            " failed: " +
            std::strerror(errno);

        return false;
    }

    return true;
}

}  // namespace

bool configureTcpKeepAlive(
    int socketFd,
    int idleSeconds,
    int intervalSeconds,
    int probeCount,
    std::string& error
) {
    if (!setIntOption(
            socketFd,
            SOL_SOCKET,
            SO_KEEPALIVE,
            1,
            "SO_KEEPALIVE",
            error
        )) {
        return false;
    }

#ifdef TCP_KEEPIDLE
    if (!setIntOption(
            socketFd,
            IPPROTO_TCP,
            TCP_KEEPIDLE,
            idleSeconds,
            "TCP_KEEPIDLE",
            error
        )) {
        return false;
    }
#endif

#ifdef TCP_KEEPINTVL
    if (!setIntOption(
            socketFd,
            IPPROTO_TCP,
            TCP_KEEPINTVL,
            intervalSeconds,
            "TCP_KEEPINTVL",
            error
        )) {
        return false;
    }
#endif

#ifdef TCP_KEEPCNT
    if (!setIntOption(
            socketFd,
            IPPROTO_TCP,
            TCP_KEEPCNT,
            probeCount,
            "TCP_KEEPCNT",
            error
        )) {
        return false;
    }
#endif

    return true;
}

}  // namespace minimuduo::net
