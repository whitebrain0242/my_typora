#pragma once

#include <cstdint>
#include <deque>
#include <filesystem>
#include <string>
#include <unordered_map>

struct PendingUpload {
    std::string token;
    std::string scope;
    std::string target;

    std::filesystem::path source_path;

    std::string file_name;
    std::uint64_t file_size = 0;
    std::string sha256_hex;

    std::int64_t created_at_unix_ms = 0;
};

struct IncomingDownload {
    std::uint64_t transfer_id = 0;

    std::string scope;
    std::string sender_username;
    std::string group_name;
    std::string file_name;

    std::uint64_t expected_size = 0;
    std::uint64_t received_size = 0;

    std::string sha256_hex;

    std::filesystem::path temp_path;
    std::filesystem::path final_path;
};

struct ClientState {
    std::string active_username;
    std::string pending_login_username;

    std::filesystem::path download_root;

    std::unordered_map<
        std::string,
        PendingUpload
    > pending_uploads;

    std::deque<std::string>
        upload_queue;

    std::string active_upload_token;

    std::unordered_map<
        std::uint64_t,
        IncomingDownload
    > downloads;
};
