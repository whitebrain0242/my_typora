#include "file_transfer_service.hpp"
#include "file_utils.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main() {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        "chatroom_v8_2_file_service_test";

    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);

    FileTransferService service(root, 1U);
    std::string error;

    if (!service.initialize(error)) {
        std::cerr << error << '\n';
        return EXIT_FAILURE;
    }

    const std::string token =
        "0123456789abcdef0123456789abcdef";

    std::filesystem::path temp_path;

    if (!service.begin_upload(
            token,
            temp_path,
            error
        )) {
        std::cerr << error << '\n';
        return EXIT_FAILURE;
    }

    const std::vector<unsigned char> first{
        'h', 'e', 'l', 'l', 'o', ' '
    };
    const std::vector<unsigned char> second{
        'f', 'i', 'l', 'e'
    };

    std::uint64_t offset = 0U;

    if (!service.append_upload_chunk(
            temp_path,
            0U,
            first,
            offset,
            error
        ) ||
        offset != first.size() ||
        !service.append_upload_chunk(
            temp_path,
            offset,
            second,
            offset,
            error
        ) ||
        offset != first.size() + second.size()) {
        std::cerr << error << '\n';
        return EXIT_FAILURE;
    }

    std::string expected_sha;
    if (!fileutil::sha256_file_hex(
            temp_path,
            expected_sha,
            error
        )) {
        std::cerr << error << '\n';
        return EXIT_FAILURE;
    }

    std::string relative_path;

    if (!service.finalize_upload(
            temp_path,
            token,
            "../../hello file.txt",
            offset,
            expected_sha,
            relative_path,
            error
        )) {
        std::cerr << error << '\n';
        return EXIT_FAILURE;
    }

    const std::filesystem::path final_path =
        root / relative_path;

    std::string actual_sha;

    if (!std::filesystem::is_regular_file(final_path) ||
        !fileutil::sha256_file_hex(
            final_path,
            actual_sha,
            error
        ) ||
        actual_sha != expected_sha ||
        final_path.filename().string().find(
            "hello_file.txt"
        ) == std::string::npos) {
        std::cerr
            << "file service finalize verification failed: "
            << error
            << '\n';
        return EXIT_FAILURE;
    }

    service.stop();
    std::filesystem::remove_all(root, ignored);

    std::cout
        << "file transfer storage tests passed\n";
    return EXIT_SUCCESS;
}
