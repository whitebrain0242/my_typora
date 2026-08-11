#include "config.hpp"
#include "protocol.hpp"

#include <charconv>
#include <fstream>
#include <unordered_map>

namespace {

bool parse_unsigned(
    const std::string& text,
    unsigned int& value
) {
    unsigned int parsed = 0;
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

bool load_key_values(
    const std::string& path,
    const std::string& label,
    std::unordered_map<std::string, std::string>& values,
    std::string& error
) {
    std::ifstream input(path);
    if (!input) {
        error = "cannot open " + label + " config: " + path;
        return false;
    }

    std::string line;
    std::size_t line_number = 0;

    while (std::getline(input, line)) {
        ++line_number;
        const std::string cleaned = trim(line);

        if (cleaned.empty() ||
            cleaned[0] == '#' ||
            cleaned[0] == ';') {
            continue;
        }

        const std::size_t equals = cleaned.find('=');
        if (equals == std::string::npos) {
            error =
                "invalid " +
                label +
                " config line " +
                std::to_string(line_number);
            return false;
        }

        const std::string key =
            trim(cleaned.substr(0, equals));
        const std::string value =
            trim(cleaned.substr(equals + 1));

        if (key.empty()) {
            error =
                "empty config key on line " +
                std::to_string(line_number);
            return false;
        }

        values[key] = value;
    }

    return true;
}

}  // namespace

bool load_mysql_config(
    const std::string& path,
    MySqlConfig& config,
    std::string& error
) {
    std::unordered_map<std::string, std::string> values;

    if (!load_key_values(
            path,
            "MySQL",
            values,
            error
        )) {
        return false;
    }

    if (values.count("host")) {
        config.host = values["host"];
    }
    if (values.count("user")) {
        config.user = values["user"];
    }
    if (values.count("password")) {
        config.password = values["password"];
    }
    if (values.count("database")) {
        config.database = values["database"];
    }

    if (values.count("port")) {
        unsigned int parsed = 0;
        if (!parse_unsigned(values["port"], parsed) ||
            parsed == 0U ||
            parsed > 65535U) {
            error = "invalid MySQL port";
            return false;
        }
        config.port = parsed;
    }

    if (values.count("connect_timeout_seconds")) {
        unsigned int parsed = 0;
        if (!parse_unsigned(
                values["connect_timeout_seconds"],
                parsed
            ) ||
            parsed == 0U ||
            parsed > 60U) {
            error = "invalid connect_timeout_seconds";
            return false;
        }
        config.connect_timeout_seconds = parsed;
    }

    if (config.user.empty()) {
        error = "MySQL config requires user";
        return false;
    }

    if (config.database.empty()) {
        error = "MySQL config requires database";
        return false;
    }

    return true;
}

bool load_redis_config(
    const std::string& path,
    RedisConfig& config,
    std::string& error
) {
    std::unordered_map<std::string, std::string> values;

    if (!load_key_values(
            path,
            "Redis",
            values,
            error
        )) {
        return false;
    }

    if (values.count("host")) {
        config.host = values["host"];
    }
    if (values.count("password")) {
        config.password = values["password"];
    }
    if (values.count("key_prefix")) {
        config.key_prefix = values["key_prefix"];
    }
    if (values.count("server_name")) {
        config.server_name = values["server_name"];
    }

    if (values.count("port")) {
        unsigned int parsed = 0;
        if (!parse_unsigned(values["port"], parsed) ||
            parsed == 0U ||
            parsed > 65535U) {
            error = "invalid Redis port";
            return false;
        }
        config.port = parsed;
    }

    if (values.count("database")) {
        unsigned int parsed = 0;
        if (!parse_unsigned(values["database"], parsed) ||
            parsed > 255U) {
            error = "invalid Redis database";
            return false;
        }
        config.database = parsed;
    }

    if (values.count("connect_timeout_ms")) {
        unsigned int parsed = 0;
        if (!parse_unsigned(
                values["connect_timeout_ms"],
                parsed
            ) ||
            parsed == 0U ||
            parsed > 30000U) {
            error = "invalid Redis connect_timeout_ms";
            return false;
        }
        config.connect_timeout_ms = parsed;
    }

    if (values.count("presence_ttl_seconds")) {
        unsigned int parsed = 0;
        if (!parse_unsigned(
                values["presence_ttl_seconds"],
                parsed
            ) ||
            parsed < 30U ||
            parsed > 86400U) {
            error =
                "presence_ttl_seconds must be 30-86400";
            return false;
        }
        config.presence_ttl_seconds = parsed;
    }

    if (config.host.empty()) {
        error = "Redis config requires host";
        return false;
    }

    if (config.key_prefix.empty()) {
        error = "Redis config requires key_prefix";
        return false;
    }

    if (config.server_name.empty()) {
        error = "Redis config requires server_name";
        return false;
    }

    return true;
}
