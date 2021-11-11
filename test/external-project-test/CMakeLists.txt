cmake_minimum_required(VERSION 3.12)

set(
    CMAKE_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpp17-toolchain.cmake"
    CACHE
    FILEPATH
    "Default toolchain"
)

set(
    HUNTER_CACHE_SERVERS
    "https://github.com/soramitsu/hunter-binary-cache"
    CACHE
    STRING
    "Binary cache server"
)

include("cmake/HunterGate.cmake")
# todo: automatically update along with the kagome settings
HunterGate(
    URL  "https://github.com/soramitsu/soramitsu-hunter/archive/tags/v0.23.257-soramitsu18.tar.gz"
    SHA1 "db065ce74581f07e552e8bae5ef955100caa1a01"
    LOCAL
)

project(KagomeFromExternalProject)

hunter_add_package(kagome)
find_package(kagome CONFIG REQUIRED)

add_executable(main src/main.cpp)
target_link_libraries(main
    kagome::executor
    kagome::runtime_wavm
    kagome::core_api
    kagome::trie_storage
    kagome::trie_storage_backend
    kagome::trie_storage_provider
    kagome::log_configurator
    kagome::leveldb_wrapper
    kagome::host_api_factory
    kagome::chain_spec
    kagome::crypto_store
    kagome::key_file_storage
    kagome::sr25519_provider
    kagome::ed25519_provider
    kagome::pbkdf2_provider
    kagome::hasher
    kagome::polkadot_trie_factory
    kagome::trie_serializer
    kagome::module_repository
    kagome::changes_tracker
    kagome::storage_code_provider
    kagome::block_header_repository
    kagome::runtime_upgrade_tracker
    )

set(test_name "kagome-from-external-project")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/xunit)
set(xml_output "--gtest_output=xml:${CMAKE_BINARY_DIR}/xunit/xunit-${test_name}.xml")
add_test(
        NAME ${test_name}
        COMMAND $<TARGET_FILE:${test_name}> ${xml_output}
)
set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_bin
        ARCHIVE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/test_lib
        LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/test_lib
        )
