#include "file_transfer_service.hpp"

#include "minimuduo/net/TcpConnection.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <system_error>
#include <utility>

namespace {

constexpr std::size_t kFileChunkBytes = 3072U;

std::string scope_text(
    chatroom::v9::FileTransferScope scope
) {
    switch (scope) {
        case chatroom::v9::FILE_TRANSFER_PRIVATE:
            return "PRIVATE";
        case chatroom::v9::FILE_TRANSFER_GROUP:
            return "GROUP";
        default:
            return "UNKNOWN";
    }
}

}  // namespace

FileTransferService::FileTransferService(
    std::filesystem::path storage_root,
    std::size_t worker_count
)
    : storage_root_(std::move(storage_root)),
      temp_root_(storage_root_ / "tmp"),
      files_root_(storage_root_ / "files") {
    worker_count = std::max<std::size_t>(
        1U,
        worker_count
    );

    workers_.reserve(worker_count);

    for (std::size_t index = 0U;
         index < worker_count;
         ++index) {
        workers_.emplace_back(
            [this] {
                worker_loop();
            }
        );
    }
}

FileTransferService::~FileTransferService() {
    stop();
}

bool FileTransferService::initialize(
    std::string& error
) {
    std::error_code filesystem_error;

    std::filesystem::create_directories(
        temp_root_,
        filesystem_error
    );

    if (filesystem_error) {
        error =
            "cannot create file-transfer temp directory: " +
            filesystem_error.message();
        return false;
    }

    std::filesystem::create_directories(
        files_root_,
        filesystem_error
    );

    if (filesystem_error) {
        error =
            "cannot create file-transfer storage directory: " +
            filesystem_error.message();
        return false;
    }

    return true;
}

bool FileTransferService::begin_upload(
    const std::string& token,
    std::filesystem::path& temp_path,
    std::string& error
) {
    if (!fileutil::is_valid_transfer_token(token)) {
        error = "invalid transfer token";
        return false;
    }

    temp_path =
        temp_root_ /
        (token + ".part");

    std::error_code filesystem_error;
    std::filesystem::remove(
        temp_path,
        filesystem_error
    );

    std::ofstream output(
        temp_path,
        std::ios::binary |
            std::ios::trunc
    );

    if (!output) {
        error =
            "cannot create temporary upload file";
        return false;
    }

    return true;
}

bool FileTransferService::append_upload_chunk(
    const std::filesystem::path& temp_path,
    std::uint64_t expected_offset,
    const std::vector<unsigned char>& bytes,
    std::uint64_t& accepted_offset,
    std::string& error
) {
    std::error_code filesystem_error;

    const std::uint64_t current_size =
        static_cast<std::uint64_t>(
            std::filesystem::file_size(
                temp_path,
                filesystem_error
            )
        );

    if (filesystem_error) {
        error =
            "cannot inspect temporary upload file";
        return false;
    }

    if (current_size != expected_offset) {
        error =
            "file chunk offset mismatch";
        return false;
    }

    std::ofstream output(
        temp_path,
        std::ios::binary |
            std::ios::app
    );

    if (!output) {
        error =
            "cannot append temporary upload file";
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
        error =
            "failed while writing upload chunk";
        return false;
    }

    accepted_offset =
        current_size +
        static_cast<std::uint64_t>(
            bytes.size()
        );

    return true;
}

bool FileTransferService::finalize_upload(
    const std::filesystem::path& temp_path,
    const std::string& token,
    const std::string& original_file_name,
    std::uint64_t expected_size,
    const std::string& expected_sha256_hex,
    std::string& stored_relative_path,
    std::string& error
) {
    std::error_code filesystem_error;

    const std::uint64_t actual_size =
        static_cast<std::uint64_t>(
            std::filesystem::file_size(
                temp_path,
                filesystem_error
            )
        );

    if (filesystem_error) {
        error =
            "cannot inspect completed upload";
        return false;
    }

    if (actual_size != expected_size) {
        error =
            "uploaded file size does not match FILE_BEGIN";
        return false;
    }

    std::string actual_sha256;
    if (!fileutil::sha256_file_hex(
            temp_path,
            actual_sha256,
            error
        )) {
        return false;
    }

    std::string expected = expected_sha256_hex;
    std::transform(
        expected.begin(),
        expected.end(),
        expected.begin(),
        [](unsigned char character) {
            return static_cast<char>(
                std::tolower(character)
            );
        }
    );

    if (actual_sha256 != expected) {
        error =
            "uploaded file SHA-256 mismatch";
        return false;
    }

    const std::string safe_name =
        fileutil::sanitize_filename(
            original_file_name
        );

    const std::filesystem::path final_name =
        token + "_" + safe_name;

    const std::filesystem::path final_path =
        files_root_ / final_name;

    std::filesystem::rename(
        temp_path,
        final_path,
        filesystem_error
    );

    if (filesystem_error) {
        // Cross-device fallback.
        filesystem_error.clear();

        std::filesystem::copy_file(
            temp_path,
            final_path,
            std::filesystem::copy_options::overwrite_existing,
            filesystem_error
        );

        if (filesystem_error) {
            error =
                "cannot move completed file into storage: " +
                filesystem_error.message();
            return false;
        }

        std::filesystem::remove(
            temp_path,
            filesystem_error
        );
    }

    stored_relative_path =
        (
            std::filesystem::path("files") /
            final_name
        ).generic_string();

    return true;
}

void FileTransferService::remove_temp_file(
    const std::filesystem::path& path
) {
    std::error_code ignored;
    std::filesystem::remove(
        path,
        ignored
    );
}

void FileTransferService::deliver_async(
    std::uint64_t transfer_id,
    const FileTransferMetadata& metadata,
    const minimuduo::net::TcpConnectionPtr& connection,
    CompletionCallback completion
) {
    submit(
        [
            this,
            transfer_id,
            metadata,
            connection,
            completion = std::move(completion)
        ]() mutable {
            deliver_file(
                transfer_id,
                std::move(metadata),
                connection,
                std::move(completion)
            );
        }
    );
}

void FileTransferService::stop() {
    {
        std::lock_guard<std::mutex> lock(
            queue_mutex_
        );

        if (stopping_) {
            return;
        }

        stopping_ = true;
    }

    queue_cv_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();

    std::queue<std::function<void()>> empty;
    {
        std::lock_guard<std::mutex> lock(
            queue_mutex_
        );
        tasks_.swap(empty);
    }
}

void FileTransferService::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(
                queue_mutex_
            );

            queue_cv_.wait(
                lock,
                [this] {
                    return stopping_ ||
                           !tasks_.empty();
                }
            );

            if (stopping_ &&
                tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();
    }
}

void FileTransferService::submit(
    std::function<void()> task
) {
    {
        std::lock_guard<std::mutex> lock(
            queue_mutex_
        );

        if (stopping_) {
            return;
        }

        tasks_.push(std::move(task));
    }

    queue_cv_.notify_one();
}

void FileTransferService::deliver_file(
    std::uint64_t transfer_id,
    FileTransferMetadata metadata,
    minimuduo::net::TcpConnectionPtr connection,
    CompletionCallback completion
) {
    if (connection == nullptr ||
        !connection->connected()) {
        if (completion) {
            completion(
                false,
                "recipient connection is closed"
            );
        }
        return;
    }

    const std::filesystem::path file_path =
        storage_root_ /
        metadata.stored_relative_path();

    std::ifstream input(
        file_path,
        std::ios::binary
    );

    if (!input) {
        if (completion) {
            completion(
                false,
                "stored file is missing"
            );
        }
        return;
    }

    connection->send(
        make_offer_line(
            transfer_id,
            metadata
        )
    );

    std::array<unsigned char, kFileChunkBytes>
        buffer{};

    std::uint64_t offset = 0U;

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
            fileutil::base64_encode(chunk);

        connection->send(
            "FILE_DATA " +
            std::to_string(transfer_id) +
            " " +
            std::to_string(offset) +
            " " +
            encoded +
            "\n"
        );

        offset +=
            static_cast<std::uint64_t>(count);
    }

    if (!input.eof()) {
        if (completion) {
            completion(
                false,
                "failed while reading stored file"
            );
        }
        return;
    }

    if (offset != metadata.file_size()) {
        if (completion) {
            completion(
                false,
                "stored file size no longer matches metadata"
            );
        }
        return;
    }

    connection->send(
        "FILE_DONE " +
        std::to_string(transfer_id) +
        "\n",
        [
            completion = std::move(completion)
        ]() mutable {
            if (completion) {
                completion(true, {});
            }
        }
    );
}

std::string FileTransferService::make_offer_line(
    std::uint64_t transfer_id,
    const FileTransferMetadata& metadata
) {
    const std::vector<unsigned char> filename_bytes(
        metadata.file_name().begin(),
        metadata.file_name().end()
    );

    const std::vector<unsigned char> group_bytes(
        metadata.group_name().begin(),
        metadata.group_name().end()
    );

    const std::string encoded_group =
        group_bytes.empty()
            ? std::string("-")
            : fileutil::base64_encode(group_bytes);

    return
        "FILE_OFFER " +
        std::to_string(transfer_id) +
        " " +
        scope_text(metadata.scope()) +
        " " +
        metadata.sender_username() +
        " " +
        encoded_group +
        " " +
        fileutil::base64_encode(filename_bytes) +
        " " +
        std::to_string(metadata.file_size()) +
        " " +
        metadata.sha256_hex() +
        "\n";
}
