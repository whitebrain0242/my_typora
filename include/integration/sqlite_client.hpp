#pragma once

#include <sqlite3.h>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

struct LocalPrivateMessage {
    std::uint64_t server_message_id = 0;
    std::string account_username;
    std::string peer_username;
    std::string sender_username;
    std::string recipient_username;
    std::string content;
    std::int64_t received_at_unix_ms = 0;
    bool outgoing = false;
    bool offline_delivery = false;
};

struct LocalGroupMessage {
    std::uint64_t server_message_id = 0;
    std::string account_username;
    std::string group_name;
    std::string sender_username;
    std::string content;
    std::int64_t received_at_unix_ms = 0;
    bool outgoing = false;
    bool offline_delivery = false;
};

struct LocalFileTransfer {
    std::uint64_t server_transfer_id = 0;
    std::string account_username;
    std::string scope;
    std::string peer_username;
    std::string group_name;
    std::string sender_username;
    std::string file_name;
    std::string local_path;
    std::uint64_t file_size = 0;
    std::string sha256_hex;
    std::int64_t received_at_unix_ms = 0;
    bool outgoing = false;
};

struct LocalCacheStats {
    std::size_t private_messages = 0;
    std::size_t group_messages = 0;
    std::size_t files = 0;
};

class SqliteClient {
public:
    SqliteClient() = default;
    ~SqliteClient();

    SqliteClient(const SqliteClient&) = delete;
    SqliteClient& operator=(const SqliteClient&) = delete;

    bool open(
        const std::string& database_path,
        std::string& error
    );

    bool cache_private_message(
        const LocalPrivateMessage& message,
        std::string& error
    );

    bool cache_group_message(
        const LocalGroupMessage& message,
        std::string& error
    );

    bool recent_private_messages(
        const std::string& account_username,
        const std::string& peer_username,
        std::size_t count,
        std::vector<LocalPrivateMessage>& messages,
        std::string& error
    );

    bool recent_group_messages(
        const std::string& account_username,
        const std::string& group_name,
        std::size_t count,
        std::vector<LocalGroupMessage>& messages,
        std::string& error
    );

    bool cache_file_transfer(
        const LocalFileTransfer& file,
        std::string& error
    );

    bool recent_file_transfers(
        const std::string& account_username,
        std::size_t count,
        std::vector<LocalFileTransfer>& files,
        std::string& error
    );

    bool stats(
        const std::string& account_username,
        LocalCacheStats& stats,
        std::string& error
    );

    const std::string& database_path() const {
        return database_path_;
    }

private:
    sqlite3* database_ = nullptr;
    std::string database_path_;
    mutable std::mutex mutex_;

    bool execute(
        const std::string& sql,
        std::string& error
    );

    bool initialize_schema(std::string& error);
    void close_locked();
};
