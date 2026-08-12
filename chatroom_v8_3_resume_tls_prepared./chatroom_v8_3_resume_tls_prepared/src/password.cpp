#include "password.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
constexpr int kIterations = 210000;
constexpr std::size_t kSaltBytes = 16;
constexpr std::size_t kHashBytes = 32;

std::string to_hex(const unsigned char* data, std::size_t size) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < size; ++i) {
        output << std::setw(2) << static_cast<unsigned int>(data[i]);
    }

    return output.str();
}

bool from_hex(const std::string& text, std::vector<unsigned char>& output) {
    if (text.size() % 2 != 0) {
        return false;
    }

    output.clear();
    output.reserve(text.size() / 2);

    for (std::size_t i = 0; i < text.size(); i += 2) {
        unsigned int value = 0;
        std::istringstream input(text.substr(i, 2));
        input >> std::hex >> value;
        if (!input || value > 255) {
            return false;
        }
        output.push_back(static_cast<unsigned char>(value));
    }

    return true;
}

bool derive(
    const std::string& password,
    const unsigned char* salt,
    std::size_t salt_size,
    int iterations,
    unsigned char* output,
    std::size_t output_size
) {
    return PKCS5_PBKDF2_HMAC(
        password.data(),
        static_cast<int>(password.size()),
        salt,
        static_cast<int>(salt_size),
        iterations,
        EVP_sha256(),
        static_cast<int>(output_size),
        output
    ) == 1;
}
}

std::string hash_password_pbkdf2(const std::string& password) {
    std::array<unsigned char, kSaltBytes> salt{};
    std::array<unsigned char, kHashBytes> hash{};

    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }

    if (!derive(
        password,
        salt.data(),
        salt.size(),
        kIterations,
        hash.data(),
        hash.size()
    )) {
        throw std::runtime_error("PBKDF2 failed");
    }

    return "pbkdf2_sha256$" + std::to_string(kIterations) + "$" +
           to_hex(salt.data(), salt.size()) + "$" +
           to_hex(hash.data(), hash.size());
}

bool verify_password_pbkdf2(const std::string& password, const std::string& encoded) {
    const std::size_t first = encoded.find('$');
    const std::size_t second = encoded.find('$', first + 1);
    const std::size_t third = encoded.find('$', second + 1);

    if (first == std::string::npos || second == std::string::npos ||
        third == std::string::npos ||
        encoded.substr(0, first) != "pbkdf2_sha256") {
        return false;
    }

    int iterations = 0;
    try {
        iterations = std::stoi(encoded.substr(first + 1, second - first - 1));
    } catch (...) {
        return false;
    }

    if (iterations < 10000 || iterations > 2000000) {
        return false;
    }

    std::vector<unsigned char> salt;
    std::vector<unsigned char> expected;

    if (!from_hex(encoded.substr(second + 1, third - second - 1), salt) ||
        !from_hex(encoded.substr(third + 1), expected) ||
        salt.empty() || expected.empty()) {
        return false;
    }

    std::vector<unsigned char> actual(expected.size());
    if (!derive(
        password,
        salt.data(),
        salt.size(),
        iterations,
        actual.data(),
        actual.size()
    )) {
        return false;
    }

    return CRYPTO_memcmp(actual.data(), expected.data(), expected.size()) == 0;
}
