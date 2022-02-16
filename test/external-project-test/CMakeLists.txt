cmake_minimum_required(VERSION 3.12)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

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
    URL  https://github.com/soramitsu/soramitsu-hunter/archive/refs/tags/v0.23.257-soramitsu21.tar.gz
    SHA1 1366fb7be6bc0124b4fd54fa7aff32e99ff353ef
    LOCAL
)

#hunter_config(
#    leveldb
#    URL  https://github.com/soramitsu/leveldb/archive/c88a9e3757f795630db5cb9a1c2442a4e5ccaed3.tar.gz
#    SHA1 3af764d4596a3ca07f892c1508c917e2692d8ec1
#)

hunter_config(
    Сrc32c
    URL "https://github.com/google/crc32c/archive/1.1.1.tar.gz"
    SHA1 "46f18ed84a12c49c42dc77fff54491a9f8b9dce8")

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
    kagome::offchain_persistent_storage
    kagome::block_header_repository
    kagome::runtime_upgrade_tracker
    )
