#pragma once

#include "mysql_database.hpp"
#include "proto_types.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct IncomingFileUpload {
    std::string token;

    chatroom::v9::FileTransferScope scope =
        chatroom::v9::FILE_TRANSFER_SCOPE_UNSPECIFIED;

    std::string target;
    std::uint64_t group_id = 0;

    std::string file_name;
    std::uint64_t expected_size = 0;
    std::uint64_t received_size = 0;
    std::string sha256_hex;

    std::filesystem::path temp_path;
    std::vector<std::string> recipients;

    FileUploadResumeState resume_state;
};

struct ClientSession {
    bool logged_in = false;
    std::string username;

    std::optional<IncomingFileUpload>
        upload;

    std::unordered_set<std::uint64_t>
        file_deliveries_in_progress;

    std::unordered_map<
        std::uint64_t,
        StoredFileTransfer
    > offered_files;
};
