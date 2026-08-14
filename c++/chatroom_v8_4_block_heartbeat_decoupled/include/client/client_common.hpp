#pragma once

#include "client/client_state.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

inline constexpr std::size_t
    kClientBufferSize = 4096U;

inline constexpr std::size_t
    kDefaultLocalHistory = 20U;

inline constexpr std::size_t
    kMaxLocalHistory = 200U;

inline constexpr std::size_t
    kFileChunkBytes = 3072U;

inline constexpr std::uint64_t
    kMaxFileSize =
        20ULL * 1024ULL * 1024ULL;

std::int64_t client_now_unix_ms();

bool starts_with(
    const std::string& text,
    const std::string& prefix
);

bool parse_uint64(
    const std::string& text,
    std::uint64_t& value
);

std::string encode_text_base64(
    const std::string& text
);

bool decode_text_base64(
    const std::string& encoded,
    std::string& text,
    std::string& error
);

bool require_local_account(
    const ClientState& state
);
