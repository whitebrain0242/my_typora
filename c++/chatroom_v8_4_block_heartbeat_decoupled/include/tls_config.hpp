#pragma once

#include <string>

struct TlsServerConfig {
    bool enabled = true;

    std::string certificate_file =
        "config/tls/server.crt";

    std::string private_key_file =
        "config/tls/server.key";
};

struct TlsClientConfig {
    bool enabled = true;
    bool verify_peer = true;

    std::string ca_file =
        "config/tls/ca.crt";

    // Optional certificate identity override.
    // Empty means: verify the IP/identity passed to chat_client.
    std::string server_name;
};
