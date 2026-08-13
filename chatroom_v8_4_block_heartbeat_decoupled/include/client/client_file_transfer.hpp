#pragma once

#include "client/client_state.hpp"

#include <string>

class SqliteClient;
class TlsClientTransport;

bool load_and_resume_pending_uploads(
    TlsClientTransport& transport,
    ClientState& state,
    SqliteClient& cache
);

bool prepare_upload(
    TlsClientTransport& transport,
    ClientState& state,
    SqliteClient& cache,
    const std::string& scope,
    const std::string& target,
    const std::string& raw_path
);

bool handle_file_protocol_line(
    TlsClientTransport& transport,
    const std::string& line,
    ClientState& state,
    SqliteClient& cache
);

void preserve_partial_downloads(
    ClientState& state
);
