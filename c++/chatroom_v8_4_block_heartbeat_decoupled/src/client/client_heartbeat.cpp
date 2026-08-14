#include "client/client_heartbeat.hpp"

#include "client/client_common.hpp"
#include "client/tls_client_transport.hpp"
#include "protocol.hpp"

#include <cstdint>
#include <iostream>

ClientHeartbeat::ClientHeartbeat(
    std::chrono::milliseconds timeout
)
    : timeout_(timeout),
      last_server_activity_(
          Clock::now()
      ) {}

void ClientHeartbeat::note_server_activity() {
    last_server_activity_ =
        Clock::now();
}

bool ClientHeartbeat::consume_protocol_line(
    const std::string& line,
    TlsClientTransport& transport
) {
    const Command command =
        parse_command(line);

    if (command.name == "PING") {
        std::uint64_t nonce = 0;

        if (!parse_uint64(
                command.raw_arguments,
                nonce
            )) {
            return true;
        }

        std::string error;

        if (!transport.send(
                "PONG " +
                std::to_string(
                    nonce
                ) +
                "\n",
                error
            )) {
            std::cerr
                << "[heartbeat] failed to send PONG: "
                << error
                << '\n';
        }

        return true;
    }

    if (command.name == "PONG") {
        // Reserved for a future client-initiated heartbeat.
        return true;
    }

    return false;
}

bool ClientHeartbeat::timed_out() const {
    return
        Clock::now() -
            last_server_activity_ >=
        timeout_;
}
