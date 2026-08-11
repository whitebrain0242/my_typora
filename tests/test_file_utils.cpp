#include "file_utils.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main() {
    const std::vector<unsigned char> input{
        0x00, 0x01, 0x02, 0x41, 0x42, 0x43, 0xff
    };

    const std::string encoded =
        fileutil::base64_encode(input);

    std::vector<unsigned char> decoded;
    std::string error;

    if (!fileutil::base64_decode(
            encoded,
            decoded,
            error
        ) ||
        decoded != input) {
        std::cerr
            << "base64 round-trip failed: "
            << error
            << '\n';
        return EXIT_FAILURE;
    }

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        "chatroom_v8_2_sha_test.txt";

    {
        std::ofstream output(
            path,
            std::ios::binary |
                std::ios::trunc
        );
        output << "abc";
    }

    std::string sha256;

    if (!fileutil::sha256_file_hex(
            path,
            sha256,
            error
        ) ||
        sha256 !=
            "ba7816bf8f01cfea414140de5dae2223"
            "b00361a396177a9cb410ff61f20015ad") {
        std::cerr
            << "SHA-256 test failed: "
            << sha256
            << " "
            << error
            << '\n';
        std::filesystem::remove(path);
        return EXIT_FAILURE;
    }

    std::filesystem::remove(path);

    const std::string token =
        fileutil::make_transfer_token();

    if (!fileutil::is_valid_transfer_token(token) ||
        !fileutil::is_valid_sha256_hex(sha256) ||
        fileutil::sanitize_filename(
            "../../hello world.txt"
        ) != "hello_world.txt") {
        std::cerr
            << "file utility validation failed\n";
        return EXIT_FAILURE;
    }

    std::cout
        << "file utility tests passed\n";
    return EXIT_SUCCESS;
}
