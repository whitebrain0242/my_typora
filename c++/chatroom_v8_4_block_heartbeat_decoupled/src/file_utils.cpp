#include "file_utils.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <utility>

namespace fileutil {

std::string base64_encode(
    const std::vector<unsigned char>& bytes
) {
    if (bytes.empty()) {
        return {};
    }

    const int output_size =
        4 * static_cast<int>(
            (bytes.size() + 2U) / 3U
        );

    std::string encoded(
        static_cast<std::size_t>(output_size),
        '\0'
    );

    const int written = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(
            encoded.data()
        ),
        bytes.data(),
        static_cast<int>(bytes.size())
    );

    if (written < 0) {
        return {};
    }

    encoded.resize(
        static_cast<std::size_t>(written)
    );
    return encoded;
}

bool base64_decode(
    const std::string& encoded,
    std::vector<unsigned char>& bytes,
    std::string& error
) {
    if (encoded.empty()) {
        bytes.clear();
        return true;
    }

    if (encoded.size() % 4U != 0U) {
        error = "invalid base64 length";
        return false;
    }

    std::vector<unsigned char> decoded(
        encoded.size() / 4U * 3U
    );

    const int written = EVP_DecodeBlock(
        decoded.data(),
        reinterpret_cast<const unsigned char*>(
            encoded.data()
        ),
        static_cast<int>(encoded.size())
    );

    if (written < 0) {
        error = "invalid base64 data";
        return false;
    }

    std::size_t actual =
        static_cast<std::size_t>(written);

    if (!encoded.empty() &&
        encoded.back() == '=') {
        --actual;
    }

    if (encoded.size() >= 2U &&
        encoded[encoded.size() - 2U] == '=') {
        --actual;
    }

    decoded.resize(actual);
    bytes = std::move(decoded);
    return true;
}

bool sha256_file_hex(
    const std::filesystem::path& path,
    std::string& hex_digest,
    std::string& error
) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error =
            "cannot open file for SHA-256: " +
            path.string();
        return false;
    }

    using ContextPtr =
        std::unique_ptr<
            EVP_MD_CTX,
            decltype(&EVP_MD_CTX_free)
        >;

    ContextPtr context(
        EVP_MD_CTX_new(),
        EVP_MD_CTX_free
    );

    if (!context) {
        error = "EVP_MD_CTX_new failed";
        return false;
    }

    if (EVP_DigestInit_ex(
            context.get(),
            EVP_sha256(),
            nullptr
        ) != 1) {
        error = "EVP_DigestInit_ex failed";
        return false;
    }

    std::array<char, 64 * 1024> buffer{};

    while (input) {
        input.read(
            buffer.data(),
            static_cast<std::streamsize>(
                buffer.size()
            )
        );

        const std::streamsize count =
            input.gcount();

        if (count > 0 &&
            EVP_DigestUpdate(
                context.get(),
                buffer.data(),
                static_cast<std::size_t>(count)
            ) != 1) {
            error = "EVP_DigestUpdate failed";
            return false;
        }
    }

    if (!input.eof()) {
        error =
            "failed while reading file for SHA-256";
        return false;
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE>
        digest{};
    unsigned int digest_size = 0;

    if (EVP_DigestFinal_ex(
            context.get(),
            digest.data(),
            &digest_size
        ) != 1) {
        error = "EVP_DigestFinal_ex failed";
        return false;
    }

    std::ostringstream output;
    output
        << std::hex
        << std::setfill('0');

    for (unsigned int index = 0;
         index < digest_size;
         ++index) {
        output
            << std::setw(2)
            << static_cast<unsigned int>(
                   digest[index]
               );
    }

    hex_digest = output.str();
    return true;
}

std::string make_transfer_token() {
    std::array<unsigned char, 16> bytes{};

    if (RAND_bytes(
            bytes.data(),
            static_cast<int>(bytes.size())
        ) != 1) {
        return {};
    }

    std::ostringstream output;
    output
        << std::hex
        << std::setfill('0');

    for (unsigned char byte : bytes) {
        output
            << std::setw(2)
            << static_cast<unsigned int>(byte);
    }

    return output.str();
}

std::string sanitize_filename(
    const std::string& filename
) {
    const std::filesystem::path path(filename);
    std::string name =
        path.filename().string();

    if (name.empty() ||
        name == "." ||
        name == "..") {
        return "file.bin";
    }

    for (char& character : name) {
        const unsigned char value =
            static_cast<unsigned char>(character);

        if (std::isalnum(value) == 0 &&
            character != '.' &&
            character != '_' &&
            character != '-') {
            character = '_';
        }
    }

    if (name.size() > 180U) {
        name.resize(180U);
    }

    return name.empty()
        ? std::string("file.bin")
        : name;
}

bool is_valid_transfer_token(
    const std::string& token
) {
    if (token.size() != 32U) {
        return false;
    }

    return std::all_of(
        token.begin(),
        token.end(),
        [](unsigned char character) {
            return std::isxdigit(character) != 0;
        }
    );
}

bool is_valid_sha256_hex(
    const std::string& value
) {
    if (value.size() != 64U) {
        return false;
    }

    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character) {
            return std::isxdigit(character) != 0;
        }
    );
}

}  // namespace fileutil
