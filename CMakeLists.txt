cmake_minimum_required(VERSION 3.16)
project(chatroom_v8_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED IMPORTED_TARGET mysqlclient)
pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)

find_package(OpenSSL REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

function(chat_warnings target)
    target_compile_options(
        ${target}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endfunction()

set(CHAT_PROTO_FILES
    proto/friend_event.proto
    proto/chat_message.proto
    proto/group_message.proto
    proto/file_transfer.proto
)

add_library(chat_proto STATIC
    ${CHAT_PROTO_FILES}
)

target_link_libraries(
    chat_proto
    PUBLIC
        protobuf::libprotobuf
)

set(CHAT_PROTO_GENERATED_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

file(MAKE_DIRECTORY
    ${CHAT_PROTO_GENERATED_DIR}
)

protobuf_generate(
    TARGET chat_proto
    LANGUAGE cpp
    IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/proto
    PROTOC_OUT_DIR ${CHAT_PROTO_GENERATED_DIR}
    APPEND_PATH
)

target_include_directories(
    chat_proto
    PUBLIC
        ${CHAT_PROTO_GENERATED_DIR}
)


add_library(minimuduo STATIC
    src/minimuduo/net/Buffer.cpp
    src/minimuduo/net/Channel.cpp
    src/minimuduo/net/Poller.cpp
    src/minimuduo/net/EventLoop.cpp
    src/minimuduo/net/EventLoopThread.cpp
    src/minimuduo/net/EventLoopThreadPool.cpp
    src/minimuduo/net/Acceptor.cpp
    src/minimuduo/net/TcpConnection.cpp
    src/minimuduo/net/TcpServer.cpp
)

target_include_directories(minimuduo PUBLIC include)
target_link_libraries(minimuduo PUBLIC Threads::Threads)
chat_warnings(minimuduo)

add_library(chat_core
    src/protocol.cpp
    src/config.cpp
    src/password.cpp
    src/file_utils.cpp
    src/file_transfer_service.cpp
    src/mysql_database.cpp
    src/redis_client.cpp
    src/chat_server.cpp
)

target_include_directories(chat_core PUBLIC include)
target_link_libraries(
    chat_core
    PUBLIC
        minimuduo
        chat_proto
        PkgConfig::MYSQL
        PkgConfig::HIREDIS
        OpenSSL::Crypto
        Threads::Threads
)
chat_warnings(chat_core)

add_executable(chat_server src/server_main.cpp)
target_link_libraries(chat_server PRIVATE chat_core)
chat_warnings(chat_server)

add_executable(chat_client
    src/client.cpp
    src/protocol.cpp
    src/file_utils.cpp
    src/sqlite_client.cpp
)
target_include_directories(chat_client PRIVATE include)
target_link_libraries(
    chat_client
    PRIVATE
        PkgConfig::SQLITE3
        OpenSSL::Crypto
)
chat_warnings(chat_client)

enable_testing()

add_executable(test_proto
    tests/test_proto.cpp
)
target_include_directories(test_proto PRIVATE include)
target_link_libraries(test_proto PRIVATE chat_proto)
chat_warnings(test_proto)
add_test(NAME official_protobuf COMMAND test_proto)

add_executable(test_password
    tests/test_password.cpp
    src/password.cpp
)
target_include_directories(test_password PRIVATE include)
target_link_libraries(test_password PRIVATE OpenSSL::Crypto)
chat_warnings(test_password)
add_test(NAME password_hash COMMAND test_password)

add_executable(test_file_utils
    tests/test_file_utils.cpp
    src/file_utils.cpp
)
target_include_directories(test_file_utils PRIVATE include)
target_link_libraries(test_file_utils PRIVATE OpenSSL::Crypto)
chat_warnings(test_file_utils)
add_test(NAME file_utils COMMAND test_file_utils)

add_executable(test_file_transfer_storage
    tests/test_file_transfer_storage.cpp
    src/file_transfer_service.cpp
    src/file_utils.cpp
)
target_include_directories(test_file_transfer_storage PRIVATE include)
target_link_libraries(
    test_file_transfer_storage
    PRIVATE
        minimuduo
        chat_proto
        OpenSSL::Crypto
        Threads::Threads
)
chat_warnings(test_file_transfer_storage)
add_test(NAME file_transfer_storage COMMAND test_file_transfer_storage)

add_executable(test_buffer
    tests/test_buffer.cpp
    src/minimuduo/net/Buffer.cpp
)
target_include_directories(test_buffer PRIVATE include)
chat_warnings(test_buffer)
add_test(NAME buffer COMMAND test_buffer)

add_executable(test_reactor_smoke
    tests/test_reactor_smoke.cpp
)
target_link_libraries(test_reactor_smoke PRIVATE minimuduo)
chat_warnings(test_reactor_smoke)
add_test(NAME master_sub_reactor_smoke COMMAND test_reactor_smoke)
set_tests_properties(
    master_sub_reactor_smoke
    PROPERTIES TIMEOUT 10
)

add_executable(test_reactor_distribution
    tests/test_reactor_distribution.cpp
)
target_link_libraries(test_reactor_distribution PRIVATE minimuduo)
chat_warnings(test_reactor_distribution)
add_test(
    NAME reactor_round_robin_distribution
    COMMAND test_reactor_distribution
)
set_tests_properties(
    reactor_round_robin_distribution
    PROPERTIES TIMEOUT 10
)

add_executable(test_cross_thread_send
    tests/test_cross_thread_send.cpp
)
target_link_libraries(test_cross_thread_send PRIVATE minimuduo)
chat_warnings(test_cross_thread_send)
add_test(
    NAME reactor_cross_thread_eventfd
    COMMAND test_cross_thread_send
)
set_tests_properties(
    reactor_cross_thread_eventfd
    PROPERTIES TIMEOUT 10
)

add_executable(test_sqlite_cache
    tests/test_sqlite_cache.cpp
    src/sqlite_client.cpp
)
target_include_directories(test_sqlite_cache PRIVATE include)
target_link_libraries(
    test_sqlite_cache
    PRIVATE
        PkgConfig::SQLITE3
)
chat_warnings(test_sqlite_cache)
add_test(NAME sqlite_local_cache COMMAND test_sqlite_cache)

add_executable(test_redis_client
    tests/test_redis_client.cpp
    src/redis_client.cpp
)
target_include_directories(test_redis_client PRIVATE include)
target_link_libraries(
    test_redis_client
    PRIVATE
        PkgConfig::HIREDIS
        Threads::Threads
)
chat_warnings(test_redis_client)
add_test(NAME redis_client_fake_server COMMAND test_redis_client)
set_tests_properties(
    redis_client_fake_server
    PROPERTIES TIMEOUT 10
)
