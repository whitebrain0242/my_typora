#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fileutil {

std::string base64_encode(
    const std::vector<unsigned char>& bytes
);

bool base64_decode(
    const std::string& encoded,
    std::vector<unsigned char>& bytes,
    std::string& error
);

bool sha256_file_hex(
    const std::filesystem::path& path,
    std::string& hex_digest,
    std::string& error
);

std::string make_transfer_token();

std::string sanitize_filename(
    const std::string& filename
);

bool is_valid_transfer_token(
    const std::string& token
);

bool is_valid_sha256_hex(
    const std::string& value
);

}  // namespace fileutil
