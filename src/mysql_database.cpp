#include "mysql_database.hpp"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <utility>

namespace {

using ResultPtr =
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

std::string mysql_error_text(MYSQL* connection) {
    return connection != nullptr
        ? mysql_error(connection)
        : "MySQL connection is null";
}

class MySqlThreadGuard {
public:
    MySqlThreadGuard() {
        (void)mysql_thread_init();
    }

    ~MySqlThreadGuard() {
        mysql_thread_end();
    }
};

void ensure_mysql_thread() {
    thread_local MySqlThreadGuard guard;
    (void)guard;
}


}  // namespace

MySqlDatabase::~MySqlDatabase() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_ != nullptr) {
        mysql_close(connection_);
        connection_ = nullptr;
    }
}

bool MySqlDatabase::connect(
    const MySqlConfig& config,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    if (connection_ != nullptr) {
        mysql_close(connection_);
        connection_ = nullptr;
    }

    connection_ = mysql_init(nullptr);
    if (connection_ == nullptr) {
        error = "mysql_init failed";
        return false;
    }

    mysql_options(
        connection_,
        MYSQL_OPT_CONNECT_TIMEOUT,
        &config.connect_timeout_seconds
    );

    if (mysql_real_connect(
            connection_,
            config.host.c_str(),
            config.user.c_str(),
            config.password.c_str(),
            config.database.c_str(),
            config.port,
            nullptr,
            CLIENT_MULTI_STATEMENTS
        ) == nullptr) {
        error = mysql_error_text(connection_);
        mysql_close(connection_);
        connection_ = nullptr;
        return false;
    }

    if (mysql_set_character_set(connection_, "utf8mb4") != 0) {
        error = mysql_error_text(connection_);
        mysql_close(connection_);
        connection_ = nullptr;
        return false;
    }

    return true;
}

bool MySqlDatabase::ping(std::string& error) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    if (connection_ == nullptr || mysql_ping(connection_) != 0) {
        error = mysql_error_text(connection_);
        return false;
    }

    return true;
}

std::string MySqlDatabase::escape(const std::string& value) {
    if (connection_ == nullptr) {
        return {};
    }

    std::string escaped(value.size() * 2U + 1U, '\0');

    const unsigned long size = mysql_real_escape_string(
        connection_,
        escaped.data(),
        value.data(),
        static_cast<unsigned long>(value.size())
    );

    escaped.resize(size);
    return escaped;
}

bool MySqlDatabase::execute(
    const std::string& sql,
    std::string& error
) {
    if (connection_ == nullptr) {
        error = "database is not connected";
        return false;
    }

    if (mysql_real_query(
            connection_,
            sql.data(),
            static_cast<unsigned long>(sql.size())
        ) != 0) {
        error = mysql_error_text(connection_);
        return false;
    }

    return true;
}

bool MySqlDatabase::begin(std::string& error) {
    return execute("START TRANSACTION", error);
}

bool MySqlDatabase::commit(std::string& error) {
    return execute("COMMIT", error);
}

void MySqlDatabase::rollback() {
    std::string ignored;
    (void)execute("ROLLBACK", ignored);
}

std::pair<std::string, std::string> MySqlDatabase::normalize_pair(
    const std::string& left,
    const std::string& right
) {
    if (left <= right) {
        return {left, right};
    }

    return {right, left};
}

bool MySqlDatabase::user_exists(
    const std::string& username,
    bool& exists,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT 1 FROM users WHERE username='" +
        escape(username) +
        "' LIMIT 1";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    exists = mysql_num_rows(result.get()) > 0U;
    return true;
}

bool MySqlDatabase::create_user(
    const std::string& username,
    const std::string& password_hash,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "INSERT INTO users(username,password_hash) VALUES('" +
        escape(username) +
        "','" +
        escape(password_hash) +
        "')";

    return execute(sql, error);
}

bool MySqlDatabase::get_password_hash(
    const std::string& username,
    std::optional<std::string>& password_hash,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT password_hash FROM users WHERE username='" +
        escape(username) +
        "' LIMIT 1";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result.get());
    if (row == nullptr) {
        password_hash.reset();
        return true;
    }

    unsigned long* lengths = mysql_fetch_lengths(result.get());
    password_hash = std::string(row[0], lengths[0]);
    return true;
}

bool MySqlDatabase::are_friends(
    const std::string& left,
    const std::string& right,
    bool& result_value,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const auto pair = normalize_pair(left, right);

    const std::string sql =
        "SELECT 1 FROM friendships WHERE user_low='" +
        escape(pair.first) +
        "' AND user_high='" +
        escape(pair.second) +
        "' LIMIT 1";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    result_value = mysql_num_rows(result.get()) > 0U;
    return true;
}

bool MySqlDatabase::has_friend_request(
    const std::string& requester,
    const std::string& target,
    bool& result_value,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT 1 FROM friend_requests "
        "WHERE requester_username='" +
        escape(requester) +
        "' AND target_username='" +
        escape(target) +
        "' LIMIT 1";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    result_value = mysql_num_rows(result.get()) > 0U;
    return true;
}

bool MySqlDatabase::add_friend_request(
    const std::string& requester,
    const std::string& target,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "INSERT INTO friend_requests("
        "requester_username,target_username) VALUES('" +
        escape(requester) +
        "','" +
        escape(target) +
        "')";

    return execute(sql, error);
}

bool MySqlDatabase::accept_friend_request(
    const std::string& requester,
    const std::string& target,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    if (!begin(error)) {
        return false;
    }

    const std::string delete_sql =
        "DELETE FROM friend_requests "
        "WHERE requester_username='" +
        escape(requester) +
        "' AND target_username='" +
        escape(target) +
        "'";

    if (!execute(delete_sql, error) ||
        mysql_affected_rows(connection_) != 1U) {
        if (error.empty()) {
            error = "friend request does not exist";
        }
        rollback();
        return false;
    }

    const auto pair = normalize_pair(requester, target);

    const std::string insert_sql =
        "INSERT INTO friendships(user_low,user_high) VALUES('" +
        escape(pair.first) +
        "','" +
        escape(pair.second) +
        "')";

    if (!execute(insert_sql, error)) {
        rollback();
        return false;
    }

    if (!commit(error)) {
        rollback();
        return false;
    }

    return true;
}

bool MySqlDatabase::reject_friend_request(
    const std::string& requester,
    const std::string& target,
    bool& removed,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "DELETE FROM friend_requests "
        "WHERE requester_username='" +
        escape(requester) +
        "' AND target_username='" +
        escape(target) +
        "'";

    if (!execute(sql, error)) {
        return false;
    }

    removed = mysql_affected_rows(connection_) == 1U;
    return true;
}

bool MySqlDatabase::remove_friendship(
    const std::string& left,
    const std::string& right,
    bool& removed,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const auto pair = normalize_pair(left, right);

    const std::string sql =
        "DELETE FROM friendships WHERE user_low='" +
        escape(pair.first) +
        "' AND user_high='" +
        escape(pair.second) +
        "'";

    if (!execute(sql, error)) {
        return false;
    }

    removed = mysql_affected_rows(connection_) == 1U;
    return true;
}

bool MySqlDatabase::list_friends(
    const std::string& username,
    std::vector<std::string>& friends,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string escaped = escape(username);

    const std::string sql =
        "SELECT CASE WHEN user_low='" +
        escaped +
        "' THEN user_high ELSE user_low END AS friend_name "
        "FROM friendships WHERE user_low='" +
        escaped +
        "' OR user_high='" +
        escaped +
        "' ORDER BY friend_name";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    friends.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());
        friends.emplace_back(row[0], lengths[0]);
    }

    return true;
}

bool MySqlDatabase::list_incoming_requests(
    const std::string& username,
    std::vector<std::string>& users,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT requester_username FROM friend_requests "
        "WHERE target_username='" +
        escape(username) +
        "' ORDER BY requester_username";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    users.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());
        users.emplace_back(row[0], lengths[0]);
    }

    return true;
}

bool MySqlDatabase::list_outgoing_requests(
    const std::string& username,
    std::vector<std::string>& users,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT target_username FROM friend_requests "
        "WHERE requester_username='" +
        escape(username) +
        "' ORDER BY target_username";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    users.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());
        users.emplace_back(row[0], lengths[0]);
    }

    return true;
}

bool MySqlDatabase::add_friend_event(
    const FriendEventPayload& event,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    std::string payload;
    if (!event.SerializeToString(&payload)) {
        error = "official protobuf failed to serialize FriendEventPayload";
        return false;
    }

    const std::string sql =
        "INSERT INTO friend_events("
        "event_type,actor_username,target_username,"
        "occurred_at_unix_ms,payload) VALUES(" +
        std::to_string(static_cast<std::uint32_t>(event.type())) +
        ",'" +
        escape(event.actor_username()) +
        "','" +
        escape(event.target_username()) +
        "'," +
        std::to_string(event.occurred_at_unix_ms()) +
        ",'" +
        escape(payload) +
        "')";

    return execute(sql, error);
}

bool MySqlDatabase::insert_message_locked(
    const ChatMessagePayload& payload,
    std::uint64_t& message_id,
    std::string& error
) {
    std::string bytes;
    if (!payload.SerializeToString(&bytes)) {
        error = "official protobuf failed to serialize ChatMessagePayload";
        return false;
    }

    const std::string recipient_sql =
        payload.recipient_username().empty()
            ? "NULL"
            : "'" + escape(payload.recipient_username()) + "'";

    const std::string sql =
        "INSERT INTO messages("
        "message_type,sender_username,recipient_username,"
        "created_at_unix_ms,payload) VALUES(" +
        std::to_string(static_cast<std::uint32_t>(payload.type())) +
        ",'" +
        escape(payload.sender_username()) +
        "'," +
        recipient_sql +
        "," +
        std::to_string(payload.created_at_unix_ms()) +
        ",'" +
        escape(bytes) +
        "')";

    if (!execute(sql, error)) {
        return false;
    }

    message_id =
        static_cast<std::uint64_t>(mysql_insert_id(connection_));
    return true;
}

bool MySqlDatabase::add_message(
    const ChatMessagePayload& payload,
    std::uint64_t& message_id,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);
    return insert_message_locked(payload, message_id, error);
}

bool MySqlDatabase::add_private_message_with_delivery(
    const ChatMessagePayload& payload,
    std::uint64_t& message_id,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    if (payload.type() != chatroom::v7::PRIVATE ||
        payload.recipient_username().empty()) {
        error = "private delivery requires a private message recipient";
        return false;
    }

    if (!begin(error)) {
        return false;
    }

    if (!insert_message_locked(payload, message_id, error)) {
        rollback();
        return false;
    }

    const std::string delivery_sql =
        "INSERT INTO private_message_deliveries("
        "message_id,recipient_username,delivered_at_unix_ms) VALUES(" +
        std::to_string(message_id) +
        ",'" +
        escape(payload.recipient_username()) +
        "',NULL)";

    if (!execute(delivery_sql, error)) {
        rollback();
        return false;
    }

    if (!commit(error)) {
        rollback();
        return false;
    }

    return true;
}

bool MySqlDatabase::recent_public_messages(
    std::size_t count,
    std::vector<StoredMessage>& messages,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT id,payload FROM messages WHERE message_type=1 "
        "ORDER BY id DESC LIMIT " +
        std::to_string(count);

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    messages.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());

        StoredMessage message;
        message.id = std::strtoull(row[0], nullptr, 10);

        if (!message.payload.ParseFromArray(
                row[1],
                static_cast<int>(lengths[1])
            )) {
            error = "failed to decode a public message payload";
            return false;
        }

        messages.push_back(std::move(message));
    }

    std::reverse(messages.begin(), messages.end());
    return true;
}

bool MySqlDatabase::recent_private_messages(
    const std::string& user_a,
    const std::string& user_b,
    std::size_t count,
    std::vector<StoredMessage>& messages,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string a = escape(user_a);
    const std::string b = escape(user_b);

    const std::string sql =
        "SELECT id,payload FROM messages WHERE message_type=2 AND "
        "((sender_username='" +
        a +
        "' AND recipient_username='" +
        b +
        "') OR "
        "(sender_username='" +
        b +
        "' AND recipient_username='" +
        a +
        "')) ORDER BY id DESC LIMIT " +
        std::to_string(count);

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    messages.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());

        StoredMessage message;
        message.id = std::strtoull(row[0], nullptr, 10);

        if (!message.payload.ParseFromArray(
                row[1],
                static_cast<int>(lengths[1])
            )) {
            error = "failed to decode a private message payload";
            return false;
        }

        messages.push_back(std::move(message));
    }

    std::reverse(messages.begin(), messages.end());
    return true;
}

bool MySqlDatabase::pending_private_messages(
    const std::string& recipient,
    std::size_t count,
    std::vector<StoredMessage>& messages,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT m.id,m.payload "
        "FROM private_message_deliveries d "
        "JOIN messages m ON m.id=d.message_id "
        "WHERE d.recipient_username='" +
        escape(recipient) +
        "' AND d.delivered_at_unix_ms IS NULL "
        "ORDER BY m.id ASC LIMIT " +
        std::to_string(count);

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    messages.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());

        StoredMessage message;
        message.id = std::strtoull(row[0], nullptr, 10);

        if (!message.payload.ParseFromArray(
                row[1],
                static_cast<int>(lengths[1])
            )) {
            error = "failed to decode an offline private message";
            return false;
        }

        messages.push_back(std::move(message));
    }

    return true;
}

bool MySqlDatabase::mark_private_message_delivered(
    std::uint64_t message_id,
    const std::string& recipient,
    std::int64_t delivered_at_unix_ms,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "UPDATE private_message_deliveries "
        "SET delivered_at_unix_ms=" +
        std::to_string(delivered_at_unix_ms) +
        " WHERE message_id=" +
        std::to_string(message_id) +
        " AND recipient_username='" +
        escape(recipient) +
        "' AND delivered_at_unix_ms IS NULL";

    return execute(sql, error);
}

bool MySqlDatabase::group_id_by_name(
    const std::string& group_name,
    std::optional<std::uint64_t>& group_id,
    std::string& error
) {
    const std::string sql =
        "SELECT group_id FROM chat_groups WHERE group_name='" +
        escape(group_name) +
        "' LIMIT 1";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result.get());
    if (row == nullptr) {
        group_id.reset();
        return true;
    }

    group_id = std::strtoull(row[0], nullptr, 10);
    return true;
}

bool MySqlDatabase::create_group(
    const std::string& group_name,
    const std::string& owner_username,
    std::uint64_t& group_id,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    if (!begin(error)) {
        return false;
    }

    const std::string group_sql =
        "INSERT INTO chat_groups(group_name,owner_username) VALUES('" +
        escape(group_name) +
        "','" +
        escape(owner_username) +
        "')";

    if (!execute(group_sql, error)) {
        rollback();
        return false;
    }

    group_id =
        static_cast<std::uint64_t>(mysql_insert_id(connection_));

    const std::string member_sql =
        "INSERT INTO group_members("
        "group_id,username,member_role) VALUES(" +
        std::to_string(group_id) +
        ",'" +
        escape(owner_username) +
        "',1)";

    if (!execute(member_sql, error)) {
        rollback();
        return false;
    }

    if (!commit(error)) {
        rollback();
        return false;
    }

    return true;
}

bool MySqlDatabase::get_group(
    const std::string& group_name,
    std::optional<GroupInfo>& group,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT group_id,group_name,owner_username "
        "FROM chat_groups WHERE group_name='" +
        escape(group_name) +
        "' LIMIT 1";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result.get());
    if (row == nullptr) {
        group.reset();
        return true;
    }

    unsigned long* lengths = mysql_fetch_lengths(result.get());

    GroupInfo info;
    info.id = std::strtoull(row[0], nullptr, 10);
    info.name.assign(row[1], lengths[1]);
    info.owner_username.assign(row[2], lengths[2]);

    group = std::move(info);
    return true;
}

bool MySqlDatabase::dissolve_group(
    const std::string& group_name,
    const std::string& owner_username,
    bool& removed,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "DELETE FROM chat_groups WHERE group_name='" +
        escape(group_name) +
        "' AND owner_username='" +
        escape(owner_username) +
        "'";

    if (!execute(sql, error)) {
        return false;
    }

    removed = mysql_affected_rows(connection_) == 1U;
    return true;
}

bool MySqlDatabase::get_group_role(
    const std::string& group_name,
    const std::string& username,
    std::optional<GroupRole>& role,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT gm.member_role "
        "FROM group_members gm "
        "JOIN chat_groups g ON g.group_id=gm.group_id "
        "WHERE g.group_name='" +
        escape(group_name) +
        "' AND gm.username='" +
        escape(username) +
        "' LIMIT 1";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result.get());
    if (row == nullptr) {
        role.reset();
        return true;
    }

    role = static_cast<GroupRole>(
        std::strtoul(row[0], nullptr, 10)
    );

    return true;
}

bool MySqlDatabase::list_user_groups(
    const std::string& username,
    std::vector<GroupMembership>& groups,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT g.group_id,g.group_name,g.owner_username,gm.member_role "
        "FROM group_members gm "
        "JOIN chat_groups g ON g.group_id=gm.group_id "
        "WHERE gm.username='" +
        escape(username) +
        "' ORDER BY g.group_name";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    groups.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());

        GroupMembership membership;
        membership.group.id =
            std::strtoull(row[0], nullptr, 10);
        membership.group.name.assign(row[1], lengths[1]);
        membership.group.owner_username.assign(row[2], lengths[2]);
        membership.role = static_cast<GroupRole>(
            std::strtoul(row[3], nullptr, 10)
        );

        groups.push_back(std::move(membership));
    }

    return true;
}

bool MySqlDatabase::list_group_members(
    const std::string& group_name,
    std::vector<GroupMemberInfo>& members,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT gm.username,gm.member_role "
        "FROM group_members gm "
        "JOIN chat_groups g ON g.group_id=gm.group_id "
        "WHERE g.group_name='" +
        escape(group_name) +
        "' ORDER BY gm.member_role,gm.username";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    members.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());

        GroupMemberInfo member;
        member.username.assign(row[0], lengths[0]);
        member.role = static_cast<GroupRole>(
            std::strtoul(row[1], nullptr, 10)
        );

        members.push_back(std::move(member));
    }

    return true;
}

bool MySqlDatabase::list_group_member_usernames(
    const std::string& group_name,
    std::vector<std::string>& members,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT gm.username "
        "FROM group_members gm "
        "JOIN chat_groups g ON g.group_id=gm.group_id "
        "WHERE g.group_name='" +
        escape(group_name) +
        "' ORDER BY gm.username";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    members.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());
        members.emplace_back(row[0], lengths[0]);
    }

    return true;
}

bool MySqlDatabase::has_group_join_request(
    const std::string& group_name,
    const std::string& username,
    bool& exists,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT 1 "
        "FROM group_join_requests r "
        "JOIN chat_groups g ON g.group_id=r.group_id "
        "WHERE g.group_name='" +
        escape(group_name) +
        "' AND r.requester_username='" +
        escape(username) +
        "' LIMIT 1";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    exists = mysql_num_rows(result.get()) > 0U;
    return true;
}

bool MySqlDatabase::add_group_join_request(
    const std::string& group_name,
    const std::string& username,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    std::optional<std::uint64_t> group_id;
    if (!group_id_by_name(group_name, group_id, error)) {
        return false;
    }

    if (!group_id) {
        error = "group does not exist";
        return false;
    }

    const std::string sql =
        "INSERT INTO group_join_requests("
        "group_id,requester_username) VALUES(" +
        std::to_string(*group_id) +
        ",'" +
        escape(username) +
        "')";

    return execute(sql, error);
}

bool MySqlDatabase::list_group_join_requests(
    const std::string& group_name,
    std::vector<std::string>& users,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT r.requester_username "
        "FROM group_join_requests r "
        "JOIN chat_groups g ON g.group_id=r.group_id "
        "WHERE g.group_name='" +
        escape(group_name) +
        "' ORDER BY r.created_at,r.requester_username";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    users.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());
        users.emplace_back(row[0], lengths[0]);
    }

    return true;
}

bool MySqlDatabase::list_group_managers(
    const std::string& group_name,
    std::vector<std::string>& users,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT gm.username "
        "FROM group_members gm "
        "JOIN chat_groups g ON g.group_id=gm.group_id "
        "WHERE g.group_name='" +
        escape(group_name) +
        "' AND gm.member_role IN (1,2) "
        "ORDER BY gm.member_role,gm.username";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    users.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());
        users.emplace_back(row[0], lengths[0]);
    }

    return true;
}

bool MySqlDatabase::list_managed_group_request_counts(
    const std::string& username,
    std::vector<ManagedGroupRequestCount>& requests,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT g.group_name,COUNT(r.requester_username) "
        "FROM group_members gm "
        "JOIN chat_groups g ON g.group_id=gm.group_id "
        "LEFT JOIN group_join_requests r ON r.group_id=g.group_id "
        "WHERE gm.username='" +
        escape(username) +
        "' AND gm.member_role IN (1,2) "
        "GROUP BY g.group_id,g.group_name "
        "HAVING COUNT(r.requester_username)>0 "
        "ORDER BY g.group_name";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    requests.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());

        ManagedGroupRequestCount item;
        item.group_name.assign(row[0], lengths[0]);
        item.pending_count = static_cast<std::size_t>(
            std::strtoull(row[1], nullptr, 10)
        );

        requests.push_back(std::move(item));
    }

    return true;
}

bool MySqlDatabase::approve_group_join_request(
    const std::string& group_name,
    const std::string& username,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    std::optional<std::uint64_t> group_id;
    if (!group_id_by_name(group_name, group_id, error)) {
        return false;
    }

    if (!group_id) {
        error = "group does not exist";
        return false;
    }

    if (!begin(error)) {
        return false;
    }

    const std::string delete_sql =
        "DELETE FROM group_join_requests "
        "WHERE group_id=" +
        std::to_string(*group_id) +
        " AND requester_username='" +
        escape(username) +
        "'";

    if (!execute(delete_sql, error) ||
        mysql_affected_rows(connection_) != 1U) {
        if (error.empty()) {
            error = "group join request does not exist";
        }
        rollback();
        return false;
    }

    const std::string insert_sql =
        "INSERT INTO group_members("
        "group_id,username,member_role) VALUES(" +
        std::to_string(*group_id) +
        ",'" +
        escape(username) +
        "',3)";

    if (!execute(insert_sql, error)) {
        rollback();
        return false;
    }

    if (!commit(error)) {
        rollback();
        return false;
    }

    return true;
}

bool MySqlDatabase::reject_group_join_request(
    const std::string& group_name,
    const std::string& username,
    bool& removed,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    std::optional<std::uint64_t> group_id;
    if (!group_id_by_name(group_name, group_id, error)) {
        return false;
    }

    if (!group_id) {
        error = "group does not exist";
        return false;
    }

    const std::string sql =
        "DELETE FROM group_join_requests "
        "WHERE group_id=" +
        std::to_string(*group_id) +
        " AND requester_username='" +
        escape(username) +
        "'";

    if (!execute(sql, error)) {
        return false;
    }

    removed = mysql_affected_rows(connection_) == 1U;
    return true;
}

bool MySqlDatabase::set_group_member_role(
    const std::string& group_name,
    const std::string& username,
    GroupRole role,
    bool& changed,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    std::optional<std::uint64_t> group_id;
    if (!group_id_by_name(group_name, group_id, error)) {
        return false;
    }

    if (!group_id) {
        error = "group does not exist";
        return false;
    }

    const std::string sql =
        "UPDATE group_members SET member_role=" +
        std::to_string(static_cast<std::uint32_t>(role)) +
        " WHERE group_id=" +
        std::to_string(*group_id) +
        " AND username='" +
        escape(username) +
        "'";

    if (!execute(sql, error)) {
        return false;
    }

    changed = mysql_affected_rows(connection_) == 1U;
    return true;
}

bool MySqlDatabase::remove_group_member(
    const std::string& group_name,
    const std::string& username,
    bool& removed,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    std::optional<std::uint64_t> group_id;
    if (!group_id_by_name(group_name, group_id, error)) {
        return false;
    }

    if (!group_id) {
        error = "group does not exist";
        return false;
    }

    const std::string sql =
        "DELETE FROM group_members "
        "WHERE group_id=" +
        std::to_string(*group_id) +
        " AND username='" +
        escape(username) +
        "'";

    if (!execute(sql, error)) {
        return false;
    }

    removed = mysql_affected_rows(connection_) == 1U;
    return true;
}

bool MySqlDatabase::add_group_message(
    const GroupMessagePayload& payload,
    const std::vector<std::string>& recipients,
    std::uint64_t& message_id,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    if (!begin(error)) {
        return false;
    }

    std::string bytes;
    if (!payload.SerializeToString(&bytes)) {
        error = "official protobuf failed to serialize GroupMessagePayload";
        rollback();
        return false;
    }

    const std::string message_sql =
        "INSERT INTO group_messages("
        "group_id,sender_username,created_at_unix_ms,payload) VALUES(" +
        std::to_string(payload.group_id()) +
        ",'" +
        escape(payload.sender_username()) +
        "'," +
        std::to_string(payload.created_at_unix_ms()) +
        ",'" +
        escape(bytes) +
        "')";

    if (!execute(message_sql, error)) {
        rollback();
        return false;
    }

    message_id =
        static_cast<std::uint64_t>(mysql_insert_id(connection_));

    for (const std::string& recipient : recipients) {
        const std::string delivery_sql =
            "INSERT INTO group_message_deliveries("
            "message_id,recipient_username,delivered_at_unix_ms) VALUES(" +
            std::to_string(message_id) +
            ",'" +
            escape(recipient) +
            "',NULL)";

        if (!execute(delivery_sql, error)) {
            rollback();
            return false;
        }
    }

    if (!commit(error)) {
        rollback();
        return false;
    }

    return true;
}

bool MySqlDatabase::recent_group_messages(
    const std::string& group_name,
    std::size_t count,
    std::vector<StoredGroupMessage>& messages,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT gm.id,gm.payload "
        "FROM group_messages gm "
        "JOIN chat_groups g ON g.group_id=gm.group_id "
        "WHERE g.group_name='" +
        escape(group_name) +
        "' ORDER BY gm.id DESC LIMIT " +
        std::to_string(count);

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    messages.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());

        StoredGroupMessage message;
        message.id = std::strtoull(row[0], nullptr, 10);

        if (!message.payload.ParseFromArray(
                row[1],
                static_cast<int>(lengths[1])
            )) {
            error = "failed to decode a group message payload";
            return false;
        }

        messages.push_back(std::move(message));
    }

    std::reverse(messages.begin(), messages.end());
    return true;
}

bool MySqlDatabase::pending_group_messages(
    const std::string& recipient,
    std::size_t count,
    std::vector<StoredGroupMessage>& messages,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT gm.id,gm.payload "
        "FROM group_message_deliveries d "
        "JOIN group_messages gm ON gm.id=d.message_id "
        "WHERE d.recipient_username='" +
        escape(recipient) +
        "' AND d.delivered_at_unix_ms IS NULL "
        "ORDER BY gm.id ASC LIMIT " +
        std::to_string(count);

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(mysql_store_result(connection_), mysql_free_result);
    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    messages.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());

        StoredGroupMessage message;
        message.id = std::strtoull(row[0], nullptr, 10);

        if (!message.payload.ParseFromArray(
                row[1],
                static_cast<int>(lengths[1])
            )) {
            error = "failed to decode an offline group message";
            return false;
        }

        messages.push_back(std::move(message));
    }

    return true;
}

bool MySqlDatabase::mark_group_message_delivered(
    std::uint64_t message_id,
    const std::string& recipient,
    std::int64_t delivered_at_unix_ms,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "UPDATE group_message_deliveries "
        "SET delivered_at_unix_ms=" +
        std::to_string(delivered_at_unix_ms) +
        " WHERE message_id=" +
        std::to_string(message_id) +
        " AND recipient_username='" +
        escape(recipient) +
        "' AND delivered_at_unix_ms IS NULL";

    return execute(sql, error);
}

bool MySqlDatabase::add_file_transfer(
    const FileTransferMetadata& metadata,
    const std::vector<std::string>& recipients,
    std::uint64_t& transfer_id,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    if (recipients.empty()) {
        error = "file transfer requires at least one recipient";
        return false;
    }

    std::string bytes;
    if (!metadata.SerializeToString(&bytes)) {
        error =
            "official protobuf failed to serialize FileTransferMetadata";
        return false;
    }

    if (!begin(error)) {
        return false;
    }

    const std::string recipient_sql =
        metadata.recipient_username().empty()
            ? "NULL"
            : "'" +
                  escape(metadata.recipient_username()) +
                  "'";

    const std::string group_id_sql =
        metadata.group_id() == 0U
            ? "NULL"
            : std::to_string(metadata.group_id());

    const std::string insert_sql =
        "INSERT INTO file_transfers("
        "transfer_token,scope_type,sender_username,recipient_username,group_id,"
        "file_name,file_size,sha256_hex,stored_relative_path,"
        "created_at_unix_ms,metadata) VALUES('" +
        escape(metadata.transfer_token()) +
        "'," +
        std::to_string(
            static_cast<std::uint32_t>(
                metadata.scope()
            )
        ) +
        ",'" +
        escape(metadata.sender_username()) +
        "'," +
        recipient_sql +
        "," +
        group_id_sql +
        ",'" +
        escape(metadata.file_name()) +
        "'," +
        std::to_string(metadata.file_size()) +
        ",'" +
        escape(metadata.sha256_hex()) +
        "','" +
        escape(metadata.stored_relative_path()) +
        "'," +
        std::to_string(metadata.created_at_unix_ms()) +
        ",'" +
        escape(bytes) +
        "')";

    if (!execute(insert_sql, error)) {
        rollback();
        return false;
    }

    transfer_id =
        static_cast<std::uint64_t>(
            mysql_insert_id(connection_)
        );

    for (const std::string& recipient : recipients) {
        const std::string delivery_sql =
            "INSERT INTO file_transfer_deliveries("
            "transfer_id,recipient_username,delivered_at_unix_ms) VALUES(" +
            std::to_string(transfer_id) +
            ",'" +
            escape(recipient) +
            "',NULL)";

        if (!execute(delivery_sql, error)) {
            rollback();
            return false;
        }
    }

    if (!commit(error)) {
        rollback();
        return false;
    }

    return true;
}

bool MySqlDatabase::pending_file_transfers(
    const std::string& recipient,
    std::size_t count,
    std::vector<StoredFileTransfer>& transfers,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT f.id,f.metadata "
        "FROM file_transfer_deliveries d "
        "JOIN file_transfers f ON f.id=d.transfer_id "
        "WHERE d.recipient_username='" +
        escape(recipient) +
        "' AND d.delivered_at_unix_ms IS NULL "
        "ORDER BY f.id ASC LIMIT " +
        std::to_string(count);

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(
        mysql_store_result(connection_),
        mysql_free_result
    );

    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    transfers.clear();

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths =
            mysql_fetch_lengths(result.get());

        StoredFileTransfer transfer;
        transfer.id =
            std::strtoull(row[0], nullptr, 10);

        if (!transfer.metadata.ParseFromArray(
                row[1],
                static_cast<int>(lengths[1])
            )) {
            error =
                "official protobuf failed to parse FileTransferMetadata";
            return false;
        }

        transfers.push_back(
            std::move(transfer)
        );
    }

    return true;
}


bool MySqlDatabase::file_transfer_for_recipient(
    std::uint64_t transfer_id,
    const std::string& recipient,
    std::optional<StoredFileTransfer>& transfer,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "SELECT f.id,f.metadata "
        "FROM file_transfer_deliveries d "
        "JOIN file_transfers f ON f.id=d.transfer_id "
        "WHERE d.transfer_id=" +
        std::to_string(transfer_id) +
        " AND d.recipient_username='" +
        escape(recipient) +
        "' AND d.delivered_at_unix_ms IS NULL "
        "LIMIT 1";

    if (!execute(sql, error)) {
        return false;
    }

    ResultPtr result(
        mysql_store_result(connection_),
        mysql_free_result
    );

    if (!result) {
        error = mysql_error_text(connection_);
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result.get());
    if (row == nullptr) {
        transfer.reset();
        return true;
    }

    unsigned long* lengths =
        mysql_fetch_lengths(result.get());

    StoredFileTransfer stored;
    stored.id =
        std::strtoull(row[0], nullptr, 10);

    if (!stored.metadata.ParseFromArray(
            row[1],
            static_cast<int>(lengths[1])
        )) {
        error =
            "official protobuf failed to parse FileTransferMetadata";
        return false;
    }

    transfer = std::move(stored);
    return true;
}

bool MySqlDatabase::mark_file_transfer_delivered(
    std::uint64_t transfer_id,
    const std::string& recipient,
    std::int64_t delivered_at_unix_ms,
    std::string& error
) {
    ensure_mysql_thread();
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string sql =
        "UPDATE file_transfer_deliveries "
        "SET delivered_at_unix_ms=" +
        std::to_string(delivered_at_unix_ms) +
        " WHERE transfer_id=" +
        std::to_string(transfer_id) +
        " AND recipient_username='" +
        escape(recipient) +
        "' AND delivered_at_unix_ms IS NULL";

    return execute(sql, error);
}

