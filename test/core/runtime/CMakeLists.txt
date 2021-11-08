#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(wavm)
add_subdirectory(binaryen)

addtest(wasm_result_test
    wasm_result_test.cpp
    )

addtest(storage_code_provider_test
    storage_code_provider_test.cpp
    )
target_link_libraries(storage_code_provider_test
    storage_code_provider
    logger_for_tests
    )

addtest(trie_storage_provider_test
    trie_storage_provider_test.cpp
    )
target_link_libraries(trie_storage_provider_test
    trie_storage_provider
    in_memory_storage
    trie_storage_backend
    polkadot_trie_factory
    trie_serializer
    logger_for_tests
    )

addtest(uncompress_code_test
    uncompress_code_test.cpp
    )
target_link_libraries(uncompress_code_test
    log_configurator
    storage_code_provider
    )

addtest(executor_test
    executor_test.cpp
    )
target_link_libraries(executor_test
    executor
    buffer
    basic_code_provider
    scale::scale
    logger
    log_configurator
    )

addtest(runtime_upgrade_tracker_test
    runtime_upgrade_tracker_test.cpp
    )
target_link_libraries(runtime_upgrade_tracker_test
    runtime_upgrade_tracker
    blob
    buffer
    in_memory_storage
    logger
    log_configurator
    scale::scale
    )

addtest(small_lru_cache_test
        small_lru_cache_test.cpp
        )
target_link_libraries(small_lru_cache_test
        module_repository
        blob
        )
