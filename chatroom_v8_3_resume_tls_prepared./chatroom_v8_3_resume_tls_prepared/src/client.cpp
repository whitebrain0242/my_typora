#include "config.hpp"
#include "file_utils.hpp"
#include "minimuduo/net/TlsContext.hpp"
#include "integration/sqlite_client.hpp"
#include "protocol.hpp"

#include <arpa/inet.h>

#include <algorithm>

#include <cerrno>
#include <charconv>
#include <chrono>
#include <deque>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <climits>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr std::size_t kBufferSize = 4096U;
constexpr std::size_t kDefaultLocalHistory = 20U;
constexpr std::size_t kMaxLocalHistory = 200U;
constexpr std::size_t kFileChunkBytes = 3072U;
constexpr std::uint64_t kMaxFileSize =
    20ULL * 1024ULL * 1024ULL;

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

std::int64_t now_unix_ms() {
    return std::chrono::duration_cast<
        std::chrono::milliseconds
    >(
        std::chrono::system_clock::now()
            .time_since_epoch()
    ).count();
}

bool starts_with(
    const std::string& text,
    const std::string& prefix
) {
    return text.size() >= prefix.size() &&
           text.compare(
               0,
               prefix.size(),
               prefix
           ) == 0;
}

bool parse_uint64(
    const std::string& text,
    std::uint64_t& value
) {
    if (text.empty()) {
        return false;
    }

    std::uint64_t parsed = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();

    const auto result =
        std::from_chars(begin, end, parsed);

    if (result.ec != std::errc() ||
        result.ptr != end) {
        return false;
    }

    value = parsed;
    return true;
}

std::string encode_text_base64(
    const std::string& text
) {
    const std::vector<unsigned char> bytes(
        text.begin(),
        text.end()
    );

    return fileutil::base64_encode(bytes);
}

bool decode_text_base64(
    const std::string& encoded,
    std::string& text,
    std::string& error
) {
    if (encoded == "-") {
        text.clear();
        return true;
    }

    std::vector<unsigned char> bytes;

    if (!fileutil::base64_decode(
            encoded,
            bytes,
            error
        )) {
        return false;
    }

    text.assign(
        bytes.begin(),
        bytes.end()
    );

    return true;
}

int connect_to_server(
    const std::string& ip,
    int port
) {
    const int fd =
        socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        std::cerr
            << "socket failed: "
            << std::strerror(errno)
            << '\n';
        return -1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port =
        htons(static_cast<std::uint16_t>(port));

    if (inet_pton(
            AF_INET,
            ip.c_str(),
            &address.sin_addr
        ) != 1) {
        std::cerr << "invalid IPv4 address\n";
        close(fd);
        return -1;
    }

    if (connect(
            fd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) != 0) {
        std::cerr
            << "connect failed: "
            << std::strerror(errno)
            << '\n';
        close(fd);
        return -1;
    }

    return fd;
}


bool send_all(
    SSL* ssl,
    const std::string& data
) {
    if (ssl == nullptr) {
        return false;
    }

    std::size_t offset = 0U;

    while (offset < data.size()) {
        const int write_size =
            static_cast<int>(
                std::min<std::size_t>(
                    data.size() - offset,
                    static_cast<std::size_t>(
                        INT_MAX
                    )
                )
            );

        ERR_clear_error();

        const int sent =
            SSL_write(
                ssl,
                data.data() + offset,
                write_size
            );

        if (sent > 0) {
            offset +=
                static_cast<std::size_t>(
                    sent
                );
            continue;
        }

        const int ssl_error =
            SSL_get_error(
                ssl,
                sent
            );

        if (ssl_error ==
                SSL_ERROR_WANT_READ ||
            ssl_error ==
                SSL_ERROR_WANT_WRITE) {
            continue;
        }

        std::cerr
            << "TLS send failed: "
            << minimuduo::net::
                openssl_error_text(
                    "SSL_write"
                )
            << '\n';
        return false;
    }

    return true;
}

ssize_t receive_tls(
    SSL* ssl,
    char* buffer,
    std::size_t capacity
) {
    if (ssl == nullptr ||
        capacity == 0U) {
        return -1;
    }

    ERR_clear_error();

    const int received =
        SSL_read(
            ssl,
            buffer,
            static_cast<int>(
                std::min<std::size_t>(
                    capacity,
                    static_cast<std::size_t>(
                        INT_MAX
                    )
                )
            )
        );

    if (received > 0) {
        return received;
    }

    const int ssl_error =
        SSL_get_error(
            ssl,
            received
        );

    if (ssl_error ==
        SSL_ERROR_ZERO_RETURN) {
        return 0;
    }

    if (ssl_error ==
            SSL_ERROR_WANT_READ ||
        ssl_error ==
            SSL_ERROR_WANT_WRITE) {
        errno = EAGAIN;
        return -1;
    }

    errno = EIO;
    return -1;
}

bool parse_private_message_line(
    const std::string& line,
    const std::string& active_username,
    LocalPrivateMessage& message
) {
    if (active_username.empty()) {
        return false;
    }

    bool offline = false;
    std::size_t id_begin = 0U;

    if (starts_with(line, "[offline #")) {
        offline = true;
        id_begin =
            std::string("[offline #").size();
    } else if (starts_with(line, "[#") &&
               !starts_with(line, "[#G")) {
        id_begin = 2U;
    } else {
        return false;
    }

    const std::size_t id_end =
        line.find(']', id_begin);

    if (id_end == std::string::npos ||
        !parse_uint64(
            line.substr(
                id_begin,
                id_end - id_begin
            ),
            message.server_message_id
        )) {
        return false;
    }

    const std::string from_marker =
        " [private from ";
    const std::string to_marker =
        " [private to ";

    const std::size_t marker_begin =
        id_end + 1U;
    bool outgoing = false;
    std::size_t name_begin = 0U;

    if (line.compare(
            marker_begin,
            from_marker.size(),
            from_marker
        ) == 0) {
        name_begin =
            marker_begin + from_marker.size();
    } else if (line.compare(
                   marker_begin,
                   to_marker.size(),
                   to_marker
               ) == 0) {
        outgoing = true;
        name_begin =
            marker_begin + to_marker.size();
    } else {
        return false;
    }

    const std::size_t name_end =
        line.find("] ", name_begin);

    if (name_end == std::string::npos) {
        return false;
    }

    const std::string peer =
        line.substr(
            name_begin,
            name_end - name_begin
        );

    if (peer.empty()) {
        return false;
    }

    message.account_username =
        active_username;
    message.peer_username = peer;
    message.outgoing = outgoing;
    message.offline_delivery = offline;
    message.received_at_unix_ms =
        now_unix_ms();
    message.content =
        line.substr(name_end + 2U);

    if (outgoing) {
        message.sender_username =
            active_username;
        message.recipient_username = peer;
    } else {
        message.sender_username = peer;
        message.recipient_username =
            active_username;
    }

    return true;
}

bool parse_group_message_line(
    const std::string& line,
    const std::string& active_username,
    LocalGroupMessage& message
) {
    if (active_username.empty()) {
        return false;
    }

    bool offline = false;
    std::size_t id_begin = 0U;

    if (starts_with(line, "[offline #G")) {
        offline = true;
        id_begin =
            std::string("[offline #G").size();
    } else if (starts_with(line, "[#G")) {
        id_begin = 3U;
    } else {
        return false;
    }

    const std::size_t id_end =
        line.find(']', id_begin);

    if (id_end == std::string::npos ||
        !parse_uint64(
            line.substr(
                id_begin,
                id_end - id_begin
            ),
            message.server_message_id
        )) {
        return false;
    }

    const std::string group_marker =
        " [group ";
    const std::size_t group_begin =
        id_end + 1U;

    if (line.compare(
            group_begin,
            group_marker.size(),
            group_marker
        ) != 0) {
        return false;
    }

    const std::size_t group_name_begin =
        group_begin + group_marker.size();
    const std::size_t group_name_end =
        line.find("] [", group_name_begin);

    if (group_name_end ==
        std::string::npos) {
        return false;
    }

    const std::size_t sender_begin =
        group_name_end + 3U;
    const std::size_t sender_end =
        line.find("] ", sender_begin);

    if (sender_end ==
        std::string::npos) {
        return false;
    }

    message.account_username =
        active_username;
    message.group_name = line.substr(
        group_name_begin,
        group_name_end - group_name_begin
    );
    message.sender_username = line.substr(
        sender_begin,
        sender_end - sender_begin
    );
    message.content =
        line.substr(sender_end + 2U);
    message.received_at_unix_ms =
        now_unix_ms();
    message.outgoing =
        message.sender_username ==
        active_username;
    message.offline_delivery = offline;

    return !message.group_name.empty() &&
           !message.sender_username.empty();
}

void cache_server_message(
    const std::string& line,
    const ClientState& state,
    SqliteClient& cache
) {
    if (state.active_username.empty()) {
        return;
    }

    std::string error;

    LocalPrivateMessage private_message;
    if (parse_private_message_line(
            line,
            state.active_username,
            private_message
        )) {
        if (!cache.cache_private_message(
                private_message,
                error
            )) {
            std::cerr
                << "[local sqlite error] "
                << error
                << '\n';
        }
        return;
    }

    LocalGroupMessage group_message;
    if (parse_group_message_line(
            line,
            state.active_username,
            group_message
        )) {
        if (!cache.cache_group_message(
                group_message,
                error
            )) {
            std::cerr
                << "[local sqlite error] "
                << error
                << '\n';
        }
    }
}

bool require_local_account(
    const ClientState& state
) {
    if (!state.active_username.empty()) {
        return true;
    }

    std::cout
        << "[local error] LOGIN first so the client knows "
           "which account owns the local cache/files.\n";
    return false;
}

PendingUpload pending_upload_from_local(
    const LocalPendingUpload& local
) {
    PendingUpload upload;
    upload.token =
        local.transfer_token;
    upload.scope =
        local.scope;
    upload.target =
        local.target;
    upload.source_path =
        local.source_path;
    upload.file_name =
        local.file_name;
    upload.file_size =
        local.file_size;
    upload.sha256_hex =
        local.sha256_hex;
    upload.created_at_unix_ms =
        local.created_at_unix_ms;
    return upload;
}

LocalPendingUpload local_pending_upload(
    const PendingUpload& upload,
    const std::string& account_username
) {
    LocalPendingUpload local;
    local.transfer_token =
        upload.token;
    local.account_username =
        account_username;
    local.scope =
        upload.scope;
    local.target =
        upload.target;
    local.source_path =
        upload.source_path.string();
    local.file_name =
        upload.file_name;
    local.file_size =
        upload.file_size;
    local.sha256_hex =
        upload.sha256_hex;
    local.created_at_unix_ms =
        upload.created_at_unix_ms;
    return local;
}

bool upload_source_matches(
    const PendingUpload& upload,
    std::string& error
) {
    std::error_code filesystem_error;

    if (!std::filesystem::is_regular_file(
            upload.source_path,
            filesystem_error
        )) {
        error =
            "upload source file is missing";
        return false;
    }

    const std::uint64_t size =
        static_cast<std::uint64_t>(
            std::filesystem::file_size(
                upload.source_path,
                filesystem_error
            )
        );

    if (filesystem_error) {
        error =
            "cannot inspect upload source size: " +
            filesystem_error.message();
        return false;
    }

    if (size != upload.file_size) {
        error =
            "upload source size changed since task was saved";
        return false;
    }

    std::string current_sha;

    if (!fileutil::sha256_file_hex(
            upload.source_path,
            current_sha,
            error
        )) {
        return false;
    }

    if (current_sha !=
        upload.sha256_hex) {
        error =
            "upload source SHA-256 changed since task was saved";
        return false;
    }

    return true;
}

bool send_upload_begin(
    SSL* ssl,
    const PendingUpload& upload
) {
    const std::string command =
        upload.scope == "PRIVATE"
            ? "FILE_BEGIN_PRIVATE "
            : "FILE_BEGIN_GROUP ";

    return send_all(
        ssl,
        command +
            upload.token +
            " " +
            upload.target +
            " " +
            encode_text_base64(
                upload.file_name
            ) +
            " " +
            std::to_string(
                upload.file_size
            ) +
            " " +
            upload.sha256_hex +
            "\n"
    );
}

bool start_next_queued_upload(
    SSL* ssl,
    ClientState& state,
    SqliteClient& cache
) {
    if (!state.active_upload_token.empty()) {
        return true;
    }

    while (!state.upload_queue.empty()) {
        const std::string token =
            state.upload_queue.front();

        state.upload_queue.pop_front();

        const auto iterator =
            state.pending_uploads.find(
                token
            );

        if (iterator ==
            state.pending_uploads.end()) {
            continue;
        }

        std::string error;

        if (!upload_source_matches(
                iterator->second,
                error
            )) {
            std::cerr
                << "[resume] dropping pending upload "
                << token
                << ": "
                << error
                << '\n';

            std::string sqlite_error;
            (void)cache.remove_pending_upload(
                state.active_username,
                token,
                sqlite_error
            );

            state.pending_uploads.erase(
                iterator
            );
            continue;
        }

        state.active_upload_token =
            token;

        if (!send_upload_begin(
                ssl,
                iterator->second
            )) {
            state.active_upload_token.clear();
            state.upload_queue.push_front(
                token
            );
            return false;
        }

        std::cout
            << "[resume] requesting server checkpoint for "
            << iterator->second.file_name
            << " token="
            << token
            << ".\n";

        return true;
    }

    return true;
}

bool load_and_resume_pending_uploads(
    SSL* ssl,
    ClientState& state,
    SqliteClient& cache
) {
    std::vector<LocalPendingUpload>
        saved;

    std::string error;

    if (!cache.list_pending_uploads(
            state.active_username,
            saved,
            error
        )) {
        std::cerr
            << "[local sqlite error] cannot load pending uploads: "
            << error
            << '\n';
        return true;
    }

    state.pending_uploads.clear();
    state.upload_queue.clear();
    state.active_upload_token.clear();

    for (const LocalPendingUpload& local :
         saved) {
        PendingUpload upload =
            pending_upload_from_local(
                local
            );

        state.upload_queue.push_back(
            upload.token
        );

        state.pending_uploads[
            upload.token
        ] = std::move(upload);
    }

    if (!saved.empty()) {
        std::cout
            << "[resume] found "
            << saved.size()
            << " pending upload task(s) in SQLite.\n";
    }

    return start_next_queued_upload(
        ssl,
        state,
        cache
    );
}

bool prepare_upload(
    SSL* ssl,
    ClientState& state,
    SqliteClient& cache,
    const std::string& scope,
    const std::string& target,
    const std::string& raw_path
) {
    if (!require_local_account(state)) {
        return true;
    }

    const std::filesystem::path source_path =
        std::filesystem::path(
            trim(raw_path)
        );

    std::error_code filesystem_error;

    if (source_path.empty() ||
        !std::filesystem::is_regular_file(
            source_path,
            filesystem_error
        )) {
        std::cout
            << "[local error] file does not exist or "
               "is not a regular file.\n";
        return true;
    }

    const std::uint64_t file_size =
        static_cast<std::uint64_t>(
            std::filesystem::file_size(
                source_path,
                filesystem_error
            )
        );

    if (filesystem_error) {
        std::cout
            << "[local error] cannot read file size: "
            << filesystem_error.message()
            << '\n';
        return true;
    }

    if (file_size > kMaxFileSize) {
        std::cout
            << "[local error] file exceeds 20 MiB "
               "course-project limit.\n";
        return true;
    }

    std::string sha256_hex;
    std::string error;

    if (!fileutil::sha256_file_hex(
            source_path,
            sha256_hex,
            error
        )) {
        std::cout
            << "[local error] "
            << error
            << '\n';
        return true;
    }

    const std::string token =
        fileutil::make_transfer_token();

    if (token.empty()) {
        std::cout
            << "[local error] failed to create "
               "secure transfer token.\n";
        return true;
    }

    PendingUpload upload;
    upload.token = token;
    upload.scope = scope;
    upload.target = target;
    upload.source_path =
        source_path;
    upload.file_name =
        fileutil::sanitize_filename(
            source_path.filename().string()
        );
    upload.file_size =
        file_size;
    upload.sha256_hex =
        sha256_hex;
    upload.created_at_unix_ms =
        now_unix_ms();

    if (!cache.save_pending_upload(
            local_pending_upload(
                upload,
                state.active_username
            ),
            error
        )) {
        std::cout
            << "[local sqlite error] cannot persist resumable upload task: "
            << error
            << '\n';
        return true;
    }

    state.pending_uploads[token] =
        upload;

    state.upload_queue.push_back(
        token
    );

    std::cout
        << "[local] queued resumable "
        << scope
        << " upload "
        << upload.file_name
        << " ("
        << upload.file_size
        << " bytes, SHA-256 "
        << upload.sha256_hex
        << ", token "
        << token
        << ").\n";

    return start_next_queued_upload(
        ssl,
        state,
        cache
    );
}

bool send_upload_data(
    SSL* ssl,
    const PendingUpload& upload,
    std::uint64_t start_offset
) {
    if (start_offset >
        upload.file_size) {
        std::cerr
            << "[local file error] server resume offset exceeds local file size.\n";
        return false;
    }

    std::ifstream input(
        upload.source_path,
        std::ios::binary
    );

    if (!input) {
        std::cerr
            << "[local file error] cannot reopen "
            << upload.source_path
            << '\n';

        (void)send_all(
            ssl,
            "FILE_ABORT " +
                upload.token +
                "\n"
        );

        return false;
    }

    input.seekg(
        static_cast<std::streamoff>(
            start_offset
        ),
        std::ios::beg
    );

    if (!input) {
        std::cerr
            << "[local file error] cannot seek local file to resume offset "
            << start_offset
            << ".\n";

        (void)send_all(
            ssl,
            "FILE_ABORT " +
                upload.token +
                "\n"
        );

        return false;
    }

    std::vector<unsigned char> buffer(
        kFileChunkBytes
    );

    std::uint64_t offset =
        start_offset;

    while (input) {
        input.read(
            reinterpret_cast<char*>(
                buffer.data()
            ),
            static_cast<std::streamsize>(
                buffer.size()
            )
        );

        const std::streamsize count =
            input.gcount();

        if (count <= 0) {
            break;
        }

        std::vector<unsigned char> chunk(
            buffer.begin(),
            buffer.begin() + count
        );

        const std::string encoded =
            fileutil::base64_encode(
                chunk
            );

        if (!send_all(
                ssl,
                "FILE_CHUNK " +
                    upload.token +
                    " " +
                    std::to_string(
                        offset
                    ) +
                    " " +
                    encoded +
                    "\n"
            )) {
            return false;
        }

        offset +=
            static_cast<std::uint64_t>(
                count
            );
    }

    if (!input.eof() ||
        offset != upload.file_size) {
        std::cerr
            << "[local file error] file changed or "
               "read failed during upload; task remains in SQLite.\n";

        (void)send_all(
            ssl,
            "FILE_ABORT " +
            upload.token +
            "\n"
        );

        return false;
    }

    return send_all(
        ssl,
        "FILE_END " +
        upload.token +
        "\n"
    );
}

void print_local_help() {
    std::cout
        << "[local] SQLite/file commands:\n"
        << "  LOCAL_HELP\n"
        << "  LOCAL_DB\n"
        << "  LOCAL_STATS\n"
        << "  LOCAL_HISTORY_PRIVATE <username> [count]\n"
        << "  LOCAL_HISTORY_GROUP <group_name> [count]\n"
        << "  LOCAL_FILES [count]\n"
        << "  SEND_FILE <username> <path>\n"
        << "  SEND_GROUP_FILE <group_name> <path>\n"
        << "  RESUME_UPLOADS\n"
        << "[local] LOCAL_* and SEND_* are handled by "
           "chat_client; internal FILE_* lines are hidden.\n";
}

bool handle_local_command(
    SSL* ssl,
    const std::string& line,
    ClientState& state,
    SqliteClient& cache
) {
    const Command command =
        parse_command(line);

    if (command.name == "SEND_FILE" ||
        command.name == "SEND_GROUP_FILE") {
        std::string target;
        std::string path;

        if (!split_first_token(
                command.raw_arguments,
                target,
                path
            ) ||
            target.empty() ||
            path.empty()) {
            std::cout
                << "[local error] usage: "
                << (
                       command.name == "SEND_FILE"
                           ? "SEND_FILE <username> <path>"
                           : "SEND_GROUP_FILE <group_name> <path>"
                   )
                << '\n';
            return true;
        }

        (void)prepare_upload(
            ssl,
            state,
            cache,
            command.name == "SEND_FILE"
                ? "PRIVATE"
                : "GROUP",
            target,
            path
        );
        return true;
    }

    if (command.name == "RESUME_UPLOADS") {
        if (!require_local_account(state)) {
            return true;
        }

        (void)load_and_resume_pending_uploads(
            ssl,
            state,
            cache
        );
        return true;
    }

    if (!starts_with(
            command.name,
            "LOCAL_"
        )) {
        return false;
    }

    if (command.name == "LOCAL_HELP") {
        print_local_help();
        return true;
    }

    if (command.name == "LOCAL_DB") {
        std::cout
            << "[local] SQLite database: "
            << cache.database_path()
            << '\n'
            << "[local] download root: "
            << state.download_root.string()
            << '\n';
        return true;
    }

    if (!require_local_account(state)) {
        return true;
    }

    if (command.name == "LOCAL_STATS") {
        LocalCacheStats stats;
        std::string error;

        if (!cache.stats(
                state.active_username,
                stats,
                error
            )) {
            std::cout
                << "[local sqlite error] "
                << error
                << '\n';
            return true;
        }

        std::cout
            << "[local] cached private messages: "
            << stats.private_messages
            << ", cached group messages: "
            << stats.group_messages
            << ", cached files: "
            << stats.files
            << '\n';
        return true;
    }

    if (command.name ==
        "LOCAL_HISTORY_PRIVATE") {
        const std::vector<std::string> words =
            split_words(
                command.raw_arguments
            );

        if (words.empty() ||
            words.size() > 2U) {
            std::cout
                << "[local error] usage: "
                   "LOCAL_HISTORY_PRIVATE "
                   "<username> [count]\n";
            return true;
        }

        std::size_t count =
            kDefaultLocalHistory;

        if (words.size() == 2U &&
            !parse_count(
                words[1],
                1U,
                kMaxLocalHistory,
                count
            )) {
            std::cout
                << "[local error] count must be 1-200.\n";
            return true;
        }

        std::vector<LocalPrivateMessage>
            messages;
        std::string error;

        if (!cache.recent_private_messages(
                state.active_username,
                words[0],
                count,
                messages,
                error
            )) {
            std::cout
                << "[local sqlite error] "
                << error
                << '\n';
            return true;
        }

        std::cout
            << "[local private history with "
            << words[0]
            << "] "
            << messages.size()
            << " message(s):\n";

        for (const LocalPrivateMessage& message :
             messages) {
            std::cout
                << "  #"
                << message.server_message_id
                << " "
                << message.sender_username
                << " -> "
                << message.recipient_username
                << ": "
                << message.content;

            if (message.offline_delivery) {
                std::cout
                    << " [offline-delivery]";
            }

            std::cout << '\n';
        }

        return true;
    }

    if (command.name ==
        "LOCAL_HISTORY_GROUP") {
        const std::vector<std::string> words =
            split_words(
                command.raw_arguments
            );

        if (words.empty() ||
            words.size() > 2U) {
            std::cout
                << "[local error] usage: "
                   "LOCAL_HISTORY_GROUP "
                   "<group_name> [count]\n";
            return true;
        }

        std::size_t count =
            kDefaultLocalHistory;

        if (words.size() == 2U &&
            !parse_count(
                words[1],
                1U,
                kMaxLocalHistory,
                count
            )) {
            std::cout
                << "[local error] count must be 1-200.\n";
            return true;
        }

        std::vector<LocalGroupMessage>
            messages;
        std::string error;

        if (!cache.recent_group_messages(
                state.active_username,
                words[0],
                count,
                messages,
                error
            )) {
            std::cout
                << "[local sqlite error] "
                << error
                << '\n';
            return true;
        }

        std::cout
            << "[local group history "
            << words[0]
            << "] "
            << messages.size()
            << " message(s):\n";

        for (const LocalGroupMessage& message :
             messages) {
            std::cout
                << "  #G"
                << message.server_message_id
                << " ["
                << message.sender_username
                << "] "
                << message.content;

            if (message.offline_delivery) {
                std::cout
                    << " [offline-delivery]";
            }

            std::cout << '\n';
        }

        return true;
    }

    if (command.name == "LOCAL_FILES") {
        const std::vector<std::string> words =
            split_words(
                command.raw_arguments
            );

        std::size_t count =
            kDefaultLocalHistory;

        if (words.size() > 1U ||
            (
                words.size() == 1U &&
                !parse_count(
                    words[0],
                    1U,
                    kMaxLocalHistory,
                    count
                )
            )) {
            std::cout
                << "[local error] usage: "
                   "LOCAL_FILES [count], count 1-200.\n";
            return true;
        }

        std::vector<LocalFileTransfer> files;
        std::string error;

        if (!cache.recent_file_transfers(
                state.active_username,
                count,
                files,
                error
            )) {
            std::cout
                << "[local sqlite error] "
                << error
                << '\n';
            return true;
        }

        std::cout
            << "[local files] "
            << files.size()
            << " record(s):\n";

        for (const LocalFileTransfer& file :
             files) {
            std::cout
                << "  #F"
                << file.server_transfer_id
                << " ["
                << file.scope
                << "] "
                << (
                       file.outgoing
                           ? "sent"
                           : "received"
                   )
                << " "
                << file.file_name
                << " ("
                << file.file_size
                << " bytes) path="
                << file.local_path;

            if (!file.group_name.empty()) {
                std::cout
                    << " group="
                    << file.group_name;
            } else if (!file.peer_username.empty()) {
                std::cout
                    << " peer="
                    << file.peer_username;
            }

            std::cout << '\n';
        }

        return true;
    }

    std::cout
        << "[local error] unknown LOCAL_* command. "
           "Use LOCAL_HELP.\n";
    return true;
}

void remember_login_attempt(
    const std::string& line,
    ClientState& state
) {
    const Command command =
        parse_command(line);

    if (command.name != "LOGIN") {
        return;
    }

    const std::vector<std::string> words =
        split_words(
            command.raw_arguments
        );

    if (words.size() == 2U) {
        state.pending_login_username =
            words[0];
    }
}

bool begin_incoming_download(
    SSL* ssl,
    const std::vector<std::string>& words,
    ClientState& state,
    SqliteClient& cache
) {
    if (state.active_username.empty() ||
        words.size() != 7U) {
        return false;
    }

    std::uint64_t transfer_id = 0U;
    std::uint64_t file_size = 0U;

    if (!parse_uint64(
            words[0],
            transfer_id
        ) ||
        (
            words[1] != "PRIVATE" &&
            words[1] != "GROUP"
        ) ||
        !parse_uint64(
            words[5],
            file_size
        ) ||
        file_size > kMaxFileSize ||
        !fileutil::is_valid_sha256_hex(
            words[6]
        )) {
        return false;
    }

    std::string group_name;
    std::string file_name;
    std::string error;

    if (!decode_text_base64(
            words[3],
            group_name,
            error
        ) ||
        !decode_text_base64(
            words[4],
            file_name,
            error
        )) {
        std::cerr
            << "[file error] invalid FILE_OFFER base64: "
            << error
            << '\n';
        return false;
    }

    const std::filesystem::path account_dir =
        state.download_root /
        fileutil::sanitize_filename(
            state.active_username
        );

    std::error_code filesystem_error;

    std::filesystem::create_directories(
        account_dir,
        filesystem_error
    );

    if (filesystem_error) {
        std::cerr
            << "[file error] cannot create download directory: "
            << filesystem_error.message()
            << '\n';
        return false;
    }

    const std::string safe_name =
        fileutil::sanitize_filename(
            file_name
        );

    IncomingDownload download;
    download.transfer_id =
        transfer_id;
    download.scope =
        words[1];
    download.sender_username =
        words[2];
    download.group_name =
        group_name;
    download.file_name =
        safe_name;
    download.expected_size =
        file_size;
    download.sha256_hex =
        words[6];

    download.temp_path =
        account_dir /
        (
            std::to_string(
                transfer_id
            ) +
            "_" +
            safe_name +
            ".part"
        );

    download.final_path =
        account_dir /
        (
            std::to_string(
                transfer_id
            ) +
            "_" +
            safe_name
        );

    // If the final file already exists, the previous client may have
    // completed local persistence but lost the TLS acknowledgement.
    // Verify it and acknowledge without re-downloading.
    if (std::filesystem::is_regular_file(
            download.final_path,
            filesystem_error
        )) {
        filesystem_error.clear();

        const std::uint64_t final_size =
            static_cast<std::uint64_t>(
                std::filesystem::file_size(
                    download.final_path,
                    filesystem_error
                )
            );

        if (!filesystem_error &&
            final_size ==
                download.expected_size) {
            std::string final_sha;

            if (fileutil::sha256_file_hex(
                    download.final_path,
                    final_sha,
                    error
                ) &&
                final_sha ==
                    download.sha256_hex) {
                (void)send_all(
                    ssl,
                    "FILE_RECEIVED " +
                        std::to_string(
                            transfer_id
                        ) +
                        " " +
                        final_sha +
                        "\n"
                );

                std::cout
                    << "[resume] #F"
                    << transfer_id
                    << " already exists and verifies locally; "
                       "re-sent FILE_RECEIVED acknowledgement.\n";

                return true;
            }
        }
    }

    std::optional<LocalPartialDownload>
        saved_partial;

    if (!cache.get_partial_download(
            state.active_username,
            transfer_id,
            saved_partial,
            error
        )) {
        std::cerr
            << "[local sqlite error] cannot load partial download #F"
            << transfer_id
            << ": "
            << error
            << '\n';
        return false;
    }

    bool reusable_partial = false;

    if (saved_partial) {
        reusable_partial =
            saved_partial->scope ==
                download.scope &&
            saved_partial->sender_username ==
                download.sender_username &&
            saved_partial->group_name ==
                download.group_name &&
            saved_partial->file_name ==
                download.file_name &&
            saved_partial->file_size ==
                download.expected_size &&
            saved_partial->sha256_hex ==
                download.sha256_hex &&
            std::filesystem::path(
                saved_partial->temp_path
            ) ==
                download.temp_path;
    }

    if (!reusable_partial) {
        if (saved_partial) {
            std::error_code ignored;
            std::filesystem::remove(
                saved_partial->temp_path,
                ignored
            );

            std::string sqlite_error;
            (void)cache.remove_partial_download(
                state.active_username,
                transfer_id,
                sqlite_error
            );
        }

        std::ofstream output(
            download.temp_path,
            std::ios::binary |
                std::ios::trunc
        );

        if (!output) {
            std::cerr
                << "[file error] cannot create "
                << download.temp_path
                << '\n';
            return false;
        }
    } else if (!std::filesystem::exists(
                   download.temp_path,
                   filesystem_error
               )) {
        std::ofstream output(
            download.temp_path,
            std::ios::binary |
                std::ios::trunc
        );

        if (!output) {
            std::cerr
                << "[file error] cannot recreate "
                << download.temp_path
                << '\n';
            return false;
        }
    }

    filesystem_error.clear();

    const std::uint64_t local_offset =
        static_cast<std::uint64_t>(
            std::filesystem::file_size(
                download.temp_path,
                filesystem_error
            )
        );

    if (filesystem_error ||
        local_offset >
            download.expected_size) {
        std::ofstream reset(
            download.temp_path,
            std::ios::binary |
                std::ios::trunc
        );

        if (!reset) {
            std::cerr
                << "[file error] cannot reset invalid partial file.\n";
            return false;
        }

        download.received_size = 0U;
    } else {
        download.received_size =
            local_offset;
    }

    LocalPartialDownload partial;
    partial.server_transfer_id =
        transfer_id;
    partial.account_username =
        state.active_username;
    partial.scope =
        download.scope;
    partial.sender_username =
        download.sender_username;
    partial.group_name =
        download.group_name;
    partial.file_name =
        download.file_name;
    partial.temp_path =
        download.temp_path.string();
    partial.file_size =
        download.expected_size;
    partial.sha256_hex =
        download.sha256_hex;

    if (!cache.save_partial_download(
            partial,
            error
        )) {
        std::cerr
            << "[local sqlite error] cannot save partial download #F"
            << transfer_id
            << ": "
            << error
            << '\n';
        return false;
    }

    state.downloads[
        transfer_id
    ] = download;

    if (!send_all(
            ssl,
            "FILE_RESUME_REQUEST " +
                std::to_string(
                    transfer_id
                ) +
                " " +
                std::to_string(
                    download.received_size
                ) +
                "\n"
        )) {
        return false;
    }

    std::cout
        << "[file #F"
        << transfer_id
        << "] "
        << (
               download.received_size == 0U
                   ? "starting"
                   : "resuming"
           )
        << " "
        << download.scope
        << " file "
        << download.file_name
        << " at offset "
        << download.received_size
        << "/"
        << download.expected_size
        << ".\n";

    return true;
}

bool append_download_chunk(
    const std::vector<std::string>& words,
    ClientState& state
) {
    if (words.size() != 3U) {
        return false;
    }

    std::uint64_t transfer_id = 0U;
    std::uint64_t offset = 0U;

    if (!parse_uint64(
            words[0],
            transfer_id
        ) ||
        !parse_uint64(
            words[1],
            offset
        )) {
        return false;
    }

    const auto iterator =
        state.downloads.find(
            transfer_id
        );

    if (iterator ==
        state.downloads.end()) {
        return false;
    }

    IncomingDownload& download =
        iterator->second;

    if (offset !=
        download.received_size) {
        std::cerr
            << "[file error] #F"
            << transfer_id
            << " chunk offset mismatch: expected "
            << download.received_size
            << ", got "
            << offset
            << ".\n";
        return false;
    }

    std::vector<unsigned char> bytes;
    std::string error;

    if (!fileutil::base64_decode(
            words[2],
            bytes,
            error
        ) ||
        bytes.size() >
            kFileChunkBytes ||
        download.received_size +
                static_cast<std::uint64_t>(
                    bytes.size()
                ) >
            download.expected_size) {
        std::cerr
            << "[file error] #F"
            << transfer_id
            << " invalid chunk: "
            << error
            << '\n';
        return false;
    }

    std::ofstream output(
        download.temp_path,
        std::ios::binary |
            std::ios::app
    );

    if (!output) {
        std::cerr
            << "[file error] cannot append "
            << download.temp_path
            << '\n';
        return false;
    }

    if (!bytes.empty()) {
        output.write(
            reinterpret_cast<const char*>(
                bytes.data()
            ),
            static_cast<std::streamsize>(
                bytes.size()
            )
        );
    }

    if (!output) {
        std::cerr
            << "[file error] write failed for #F"
            << transfer_id
            << '\n';
        return false;
    }

    download.received_size +=
        static_cast<std::uint64_t>(
            bytes.size()
        );

    return true;
}

bool finish_incoming_download(
    SSL* ssl,
    std::uint64_t transfer_id,
    ClientState& state,
    SqliteClient& cache
) {
    const auto iterator =
        state.downloads.find(
            transfer_id
        );

    if (iterator ==
        state.downloads.end()) {
        return false;
    }

    IncomingDownload download =
        iterator->second;

    if (download.received_size !=
        download.expected_size) {
        std::cerr
            << "[file error] #F"
            << transfer_id
            << " ended with "
            << download.received_size
            << "/"
            << download.expected_size
            << " bytes; partial file is retained for resume.\n";

        (void)send_all(
            ssl,
            "FILE_RECEIVE_FAILED " +
                std::to_string(
                    transfer_id
                ) +
                "\n"
        );

        state.downloads.erase(
            iterator
        );

        return true;
    }

    std::string actual_sha256;
    std::string error;

    if (!fileutil::sha256_file_hex(
            download.temp_path,
            actual_sha256,
            error
        ) ||
        actual_sha256 !=
            download.sha256_hex) {
        std::cerr
            << "[file error] #F"
            << transfer_id
            << " SHA-256 verification failed; "
               "corrupt partial is discarded so the next attempt starts at 0.\n";

        std::error_code ignored;

        std::filesystem::remove(
            download.temp_path,
            ignored
        );

        std::string sqlite_error;
        (void)cache.remove_partial_download(
            state.active_username,
            transfer_id,
            sqlite_error
        );

        state.downloads.erase(
            iterator
        );

        (void)send_all(
            ssl,
            "FILE_RECEIVE_FAILED " +
                std::to_string(
                    transfer_id
                ) +
                "\n"
        );

        return true;
    }

    std::error_code filesystem_error;

    std::filesystem::remove(
        download.final_path,
        filesystem_error
    );

    filesystem_error.clear();

    std::filesystem::rename(
        download.temp_path,
        download.final_path,
        filesystem_error
    );

    if (filesystem_error) {
        std::cerr
            << "[file error] cannot finalize #F"
            << transfer_id
            << ": "
            << filesystem_error.message()
            << "; .part is retained for retry.\n";

        state.downloads.erase(
            iterator
        );

        (void)send_all(
            ssl,
            "FILE_RECEIVE_FAILED " +
                std::to_string(
                    transfer_id
                ) +
                "\n"
        );

        return true;
    }

    LocalFileTransfer file;
    file.server_transfer_id =
        transfer_id;
    file.account_username =
        state.active_username;
    file.scope =
        download.scope;
    file.peer_username =
        download.scope == "PRIVATE"
            ? download.sender_username
            : std::string();
    file.group_name =
        download.group_name;
    file.sender_username =
        download.sender_username;
    file.file_name =
        download.file_name;
    file.local_path =
        download.final_path.string();
    file.file_size =
        download.expected_size;
    file.sha256_hex =
        download.sha256_hex;
    file.received_at_unix_ms =
        now_unix_ms();
    file.outgoing = false;

    if (!cache.cache_file_transfer(
            file,
            error
        )) {
        std::cerr
            << "[local sqlite error] failed to cache #F"
            << transfer_id
            << ": "
            << error
            << '\n';
    }

    std::string sqlite_error;
    (void)cache.remove_partial_download(
        state.active_username,
        transfer_id,
        sqlite_error
    );

    state.downloads.erase(
        iterator
    );

    if (!send_all(
            ssl,
            "FILE_RECEIVED " +
                std::to_string(
                    transfer_id
                ) +
                " " +
                actual_sha256 +
                "\n"
        )) {
        return false;
    }

    std::cout
        << "[file #F"
        << transfer_id
        << "] received and SHA-256 verified: "
        << download.final_path.string()
        << '\n';

    return true;
}

bool handle_file_protocol_line(
    SSL* ssl,
    const std::string& line,
    ClientState& state,
    SqliteClient& cache
) {
    const Command command =
        parse_command(line);

    if (command.name == "FILE_READY") {
        const std::vector<std::string> words =
            split_words(
                command.raw_arguments
            );

        std::uint64_t start_offset = 0U;

        if (words.size() != 2U ||
            !parse_uint64(
                words[1],
                start_offset
            )) {
            return true;
        }

        const auto iterator =
            state.pending_uploads.find(
                words[0]
            );

        if (iterator ==
            state.pending_uploads.end()) {
            return true;
        }

        state.active_upload_token =
            words[0];

        std::cout
            << "[resume] server accepted upload checkpoint for "
            << iterator->second.file_name
            << "; sending from offset "
            << start_offset
            << "/"
            << iterator->second.file_size
            << ".\n";

        if (!send_upload_data(
                ssl,
                iterator->second,
                start_offset
            )) {
            std::cerr
                << "[local file error] upload stream interrupted; "
                   "SQLite task is retained for the next resume.\n";

            state.active_upload_token.clear();
        }

        return true;
    }

    if (command.name == "FILE_PAUSED") {
        const std::vector<std::string> words =
            split_words(
                command.raw_arguments
            );

        if (words.size() >= 2U) {
            std::string reason;
            std::string error;

            if (!decode_text_base64(
                    words[1],
                    reason,
                    error
                )) {
                reason =
                    "server paused file transfer";
            }

            if (state.active_upload_token ==
                words[0]) {
                state.active_upload_token.clear();
            }

            std::cout
                << "[file paused] "
                << reason
                << "\n"
                << "[resume] task remains in SQLite; "
                   "use RESUME_UPLOADS or reconnect/login.\n";
        }

        return true;
    }

    if (command.name == "FILE_REJECT") {
        const std::vector<std::string> words =
            split_words(
                command.raw_arguments
            );

        if (words.size() >= 2U) {
            std::string reason;
            std::string error;

            if (!decode_text_base64(
                    words[1],
                    reason,
                    error
                )) {
                reason =
                    "server rejected file transfer";
            }

            if (!state.active_username.empty()) {
                std::string sqlite_error;
                (void)cache.remove_pending_upload(
                    state.active_username,
                    words[0],
                    sqlite_error
                );
            }

            state.pending_uploads.erase(
                words[0]
            );

            if (state.active_upload_token ==
                words[0]) {
                state.active_upload_token.clear();
            }

            std::cout
                << "[file rejected] "
                << reason
                << '\n';

            (void)start_next_queued_upload(
                ssl,
                state,
                cache
            );
        }

        return true;
    }

    if (command.name == "FILE_UPLOAD_OK") {
        const std::vector<std::string> words =
            split_words(
                command.raw_arguments
            );

        std::uint64_t transfer_id = 0U;

        if (words.size() != 2U ||
            !parse_uint64(
                words[1],
                transfer_id
            )) {
            return true;
        }

        const auto iterator =
            state.pending_uploads.find(
                words[0]
            );

        if (iterator ==
            state.pending_uploads.end()) {
            return true;
        }

        const PendingUpload upload =
            iterator->second;

        LocalFileTransfer file;
        file.server_transfer_id =
            transfer_id;
        file.account_username =
            state.active_username;
        file.scope =
            upload.scope;
        file.peer_username =
            upload.scope == "PRIVATE"
                ? upload.target
                : std::string();
        file.group_name =
            upload.scope == "GROUP"
                ? upload.target
                : std::string();
        file.sender_username =
            state.active_username;
        file.file_name =
            upload.file_name;
        file.local_path =
            upload.source_path.string();
        file.file_size =
            upload.file_size;
        file.sha256_hex =
            upload.sha256_hex;
        file.received_at_unix_ms =
            now_unix_ms();
        file.outgoing = true;

        std::string error;

        if (!cache.cache_file_transfer(
                file,
                error
            )) {
            std::cerr
                << "[local sqlite error] "
                << error
                << '\n';
        }

        std::string sqlite_error;
        (void)cache.remove_pending_upload(
            state.active_username,
            upload.token,
            sqlite_error
        );

        state.pending_uploads.erase(
            iterator
        );

        if (state.active_upload_token ==
            upload.token) {
            state.active_upload_token.clear();
        }

        std::cout
            << "[file #F"
            << transfer_id
            << "] upload persisted on server; resumable task cleared.\n";

        (void)start_next_queued_upload(
            ssl,
            state,
            cache
        );

        return true;
    }

    if (command.name == "FILE_OFFER") {
        const std::vector<std::string> words =
            split_words(
                command.raw_arguments
            );

        if (!begin_incoming_download(
                ssl,
                words,
                state,
                cache
            )) {
            std::cerr
                << "[file error] invalid FILE_OFFER.\n";
        }

        return true;
    }

    if (command.name == "FILE_RESUME_START") {
        const std::vector<std::string> words =
            split_words(
                command.raw_arguments
            );

        std::uint64_t transfer_id = 0U;
        std::uint64_t offset = 0U;

        if (words.size() == 2U &&
            parse_uint64(
                words[0],
                transfer_id
            ) &&
            parse_uint64(
                words[1],
                offset
            )) {
            const auto iterator =
                state.downloads.find(
                    transfer_id
                );

            if (iterator !=
                    state.downloads.end() &&
                iterator->second.received_size !=
                    offset) {
                std::cerr
                    << "[file error] server resume offset "
                    << offset
                    << " does not match local offset "
                    << iterator->second.received_size
                    << " for #F"
                    << transfer_id
                    << ".\n";

                (void)send_all(
                    ssl,
                    "FILE_RECEIVE_FAILED " +
                        std::to_string(
                            transfer_id
                        ) +
                        "\n"
                );

                state.downloads.erase(
                    iterator
                );
            }
        }

        return true;
    }

    if (command.name == "FILE_DATA") {
        const std::vector<std::string> words =
            split_words(
                command.raw_arguments
            );

        if (!append_download_chunk(
                words,
                state
            )) {
            std::cerr
                << "[file error] invalid FILE_DATA; "
                   "partial file is retained when possible.\n";

            if (!words.empty()) {
                std::uint64_t transfer_id = 0U;

                if (parse_uint64(
                        words[0],
                        transfer_id
                    )) {
                    (void)send_all(
                        ssl,
                        "FILE_RECEIVE_FAILED " +
                            std::to_string(
                                transfer_id
                            ) +
                            "\n"
                    );

                    state.downloads.erase(
                        transfer_id
                    );
                }
            }
        }

        return true;
    }

    if (command.name == "FILE_DONE") {
        const std::vector<std::string> words =
            split_words(
                command.raw_arguments
            );

        std::uint64_t transfer_id = 0U;

        if (words.size() == 1U &&
            parse_uint64(
                words[0],
                transfer_id
            )) {
            (void)finish_incoming_download(
                ssl,
                transfer_id,
                state,
                cache
            );
        }

        return true;
    }

    if (command.name == "FILE_ACK_OK") {
        return true;
    }

    return false;
}

void process_server_line(
    SSL* ssl,
    const std::string& line,
    ClientState& state,
    SqliteClient& cache
) {
    if (handle_file_protocol_line(
            ssl,
            line,
            state,
            cache
        )) {
        return;
    }

    std::cout
        << line
        << '\n';

    if (starts_with(
            line,
            "[system] login successful."
        ) &&
        !state.pending_login_username.empty()) {
        state.active_username =
            state.pending_login_username;

        state.pending_login_username.clear();

        std::cout
            << "[local] SQLite cache account: "
            << state.active_username
            << '\n';

        (void)load_and_resume_pending_uploads(
            ssl,
            state,
            cache
        );
    }

    cache_server_message(
        line,
        state,
        cache
    );

    if (starts_with(
            line,
            "[system] logout successful."
        )) {
        state.active_username.clear();
        state.pending_uploads.clear();
        state.upload_queue.clear();
        state.active_upload_token.clear();
        state.downloads.clear();
    }
}

void preserve_partial_downloads(
    ClientState& state
) {
    // Deliberately do not delete *.part files. Their identity is in
    // SQLite partial_downloads and their byte length is the resume offset.
    state.downloads.clear();
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string ip = "127.0.0.1";
    int port = 9000;
    std::string sqlite_path =
        "chat_client.db";
    std::filesystem::path download_root =
        "downloads";
    std::string tls_config_path =
        "config/tls_client.conf";

    if (argc >= 2) {
        ip = argv[1];
    }

    if (argc >= 3 &&
        !parse_port(argv[2], port)) {
        std::cerr << "invalid port\n";
        return 1;
    }

    if (argc >= 4) {
        sqlite_path = argv[3];
    }

    if (argc >= 5) {
        download_root = argv[4];
    }

    if (argc >= 6) {
        tls_config_path = argv[5];
    }

    SqliteClient cache;
    std::string error;

    if (!cache.open(
            sqlite_path,
            error
        )) {
        std::cerr
            << "SQLite open failed: "
            << error
            << '\n';
        return 1;
    }

    TlsClientConfig tls_config;

    if (!load_tls_client_config(
            tls_config_path,
            tls_config,
            error
        )) {
        std::cerr
            << "TLS client config failed: "
            << error
            << '\n';
        return 1;
    }

    minimuduo::net::TlsClientContext
        tls_context;

    if (!tls_context.initialize(
            tls_config,
            error
        )) {
        std::cerr
            << "TLS client initialization failed: "
            << error
            << '\n';
        return 1;
    }

    const int socket_fd =
        connect_to_server(
            ip,
            port
        );

    if (socket_fd < 0) {
        return 1;
    }

    const std::string peer_identity =
        tls_config.server_name.empty()
            ? ip
            : tls_config.server_name;

    minimuduo::net::SslPtr ssl_session =
        tls_context.createSsl(
            socket_fd,
            peer_identity,
            error
        );

    if (!ssl_session) {
        std::cerr
            << "TLS session creation failed: "
            << error
            << '\n';
        close(socket_fd);
        return 1;
    }

    if (!tls_context.connectBlocking(
            ssl_session.get(),
            error
        )) {
        std::cerr
            << "TLS handshake failed: "
            << error
            << '\n';
        close(socket_fd);
        return 1;
    }

    SSL* ssl =
        ssl_session.get();

    ClientState state;
    state.download_root =
        std::move(download_root);

    std::cout
        << "TLS connected to "
        << ip
        << ":"
        << port
        << '\n'
        << "[tls] version: "
        << SSL_get_version(ssl)
        << ", cipher: "
        << SSL_get_cipher_name(ssl)
        << '\n'
        << "[tls] verified peer identity: "
        << peer_identity
        << '\n'
        << "[local] SQLite cache: "
        << cache.database_path()
        << '\n'
        << "[local] download root: "
        << state.download_root.string()
        << '\n'
        << "[local] Type LOCAL_HELP for local/file commands.\n";

    pollfd descriptors[2]{};
    descriptors[0].fd =
        STDIN_FILENO;
    descriptors[0].events =
        POLLIN;
    descriptors[1].fd =
        socket_fd;
    descriptors[1].events =
        POLLIN;

    bool waiting_for_server_close =
        false;

    char buffer[kBufferSize]{};
    std::string server_buffer;

    auto consume_complete_lines =
        [&]() {
            std::size_t newline = 0U;

            while ((newline =
                        server_buffer.find('\n')) !=
                   std::string::npos) {
                std::string line =
                    server_buffer.substr(
                        0,
                        newline
                    );

                server_buffer.erase(
                    0,
                    newline + 1U
                );

                if (!line.empty() &&
                    line.back() == '\r') {
                    line.pop_back();
                }

                process_server_line(
                    ssl,
                    line,
                    state,
                    cache
                );
            }

            std::cout.flush();
        };

    auto read_tls_available =
        [&]() -> bool {
            bool read_at_least_one =
                false;

            while (true) {
                const ssize_t received =
                    receive_tls(
                        ssl,
                        buffer,
                        sizeof(buffer)
                    );

                if (received > 0) {
                    read_at_least_one =
                        true;

                    server_buffer.append(
                        buffer,
                        static_cast<std::size_t>(
                            received
                        )
                    );

                    consume_complete_lines();

                    if (SSL_pending(ssl) <= 0) {
                        return true;
                    }

                    continue;
                }

                if (received == 0) {
                    return false;
                }

                if (errno == EAGAIN ||
                    errno == EINTR) {
                    return true;
                }

                std::cerr
                    << "TLS receive failed: "
                    << minimuduo::net::
                        openssl_error_text(
                            "SSL_read"
                        )
                    << '\n';

                return false;
            }

            return read_at_least_one;
        };

    bool running = true;

    while (running) {
        // Drain already-decrypted OpenSSL bytes before blocking in poll.
        if (SSL_pending(ssl) > 0) {
            if (!read_tls_available()) {
                break;
            }
        }

        const int result =
            poll(
                descriptors,
                2,
                -1
            );

        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }

            std::cerr
                << "poll failed: "
                << std::strerror(errno)
                << '\n';
            break;
        }

        if (!waiting_for_server_close &&
            (descriptors[0].revents &
             POLLIN)) {
            std::string line;

            if (!std::getline(
                    std::cin,
                    line
                )) {
                break;
            }

            if (handle_local_command(
                    ssl,
                    line,
                    state,
                    cache
                )) {
                continue;
            }

            remember_login_attempt(
                line,
                state
            );

            const bool quitting =
                trim(line) == "QUIT";

            if (!send_all(
                    ssl,
                    line + "\n"
                )) {
                break;
            }

            if (quitting) {
                waiting_for_server_close =
                    true;

                descriptors[0].fd =
                    -1;
            }
        }

        if (descriptors[1].revents &
            POLLIN) {
            if (!read_tls_available()) {
                running = false;
            }
        }

        if (descriptors[1].revents &
            (POLLERR |
             POLLHUP |
             POLLNVAL)) {
            // Try to drain a clean TLS close_notify first.
            if (SSL_pending(ssl) > 0) {
                (void)read_tls_available();
            }

            break;
        }
    }

    if (!server_buffer.empty()) {
        process_server_line(
            ssl,
            server_buffer,
            state,
            cache
        );
    }

    preserve_partial_downloads(
        state
    );

    ERR_clear_error();
    (void)SSL_shutdown(ssl);

    ssl_session.reset();

    close(socket_fd);

    return 0;
}
