#include "integration/sqlite_client.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main() {
    SqliteClient cache;
    std::string error;

    if (!cache.open(":memory:", error)) {
        std::cerr << error << '\n';
        return EXIT_FAILURE;
    }

    LocalPrivateMessage outgoing;
    outgoing.server_message_id = 10;
    outgoing.account_username = "alice";
    outgoing.peer_username = "bob";
    outgoing.sender_username = "alice";
    outgoing.recipient_username = "bob";
    outgoing.content = "hello";
    outgoing.received_at_unix_ms = 1000;
    outgoing.outgoing = true;

    LocalPrivateMessage incoming;
    incoming.server_message_id = 11;
    incoming.account_username = "alice";
    incoming.peer_username = "bob";
    incoming.sender_username = "bob";
    incoming.recipient_username = "alice";
    incoming.content = "reply";
    incoming.received_at_unix_ms = 2000;
    incoming.offline_delivery = true;

    if (!cache.cache_private_message(outgoing, error) ||
        !cache.cache_private_message(incoming, error)) {
        std::cerr << error << '\n';
        return EXIT_FAILURE;
    }

    LocalGroupMessage group;
    group.server_message_id = 7;
    group.account_username = "alice";
    group.group_name = "cpp";
    group.sender_username = "bob";
    group.content = "group hello";
    group.received_at_unix_ms = 3000;

    if (!cache.cache_group_message(group, error)) {
        std::cerr << error << '\n';
        return EXIT_FAILURE;
    }

    std::vector<LocalPrivateMessage> private_messages;
    if (!cache.recent_private_messages(
            "alice",
            "bob",
            20,
            private_messages,
            error
        ) ||
        private_messages.size() != 2U ||
        private_messages[0].server_message_id != 10U ||
        private_messages[1].server_message_id != 11U ||
        !private_messages[1].offline_delivery) {
        std::cerr << "private SQLite history failed: " << error << '\n';
        return EXIT_FAILURE;
    }

    std::vector<LocalGroupMessage> group_messages;
    if (!cache.recent_group_messages(
            "alice",
            "cpp",
            20,
            group_messages,
            error
        ) ||
        group_messages.size() != 1U ||
        group_messages[0].server_message_id != 7U) {
        std::cerr << "group SQLite history failed: " << error << '\n';
        return EXIT_FAILURE;
    }

    LocalFileTransfer file;
    file.server_transfer_id = 33;
    file.account_username = "alice";
    file.scope = "PRIVATE";
    file.peer_username = "bob";
    file.sender_username = "bob";
    file.file_name = "report.pdf";
    file.local_path = "downloads/alice/33_report.pdf";
    file.file_size = 4096;
    file.sha256_hex =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    file.received_at_unix_ms = 4000;
    file.outgoing = false;

    if (!cache.cache_file_transfer(file, error)) {
        std::cerr << "file SQLite cache failed: " << error << '\n';
        return EXIT_FAILURE;
    }

    std::vector<LocalFileTransfer> files;
    if (!cache.recent_file_transfers(
            "alice",
            20,
            files,
            error
        ) ||
        files.size() != 1U ||
        files[0].server_transfer_id != 33U ||
        files[0].file_name != "report.pdf") {
        std::cerr << "file SQLite history failed: " << error << '\n';
        return EXIT_FAILURE;
    }

    LocalCacheStats stats;
    if (!cache.stats("alice", stats, error) ||
        stats.private_messages != 2U ||
        stats.group_messages != 1U ||
        stats.files != 1U) {
        std::cerr << "SQLite stats failed: " << error << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "sqlite cache tests passed\n";
    return EXIT_SUCCESS;
}
