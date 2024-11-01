#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#
if ("${WASM_COMPILER}" STREQUAL "WAVM")
  add_subdirectory(wavm)
endif ()

add_subdirectory(binaryen)

addtest(allocator_test
    allocator_test.cpp
    )
target_link_libraries(allocator_test
    memory_allocator
    scale::scale
    hexutil
    )

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
    storage
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
    basic_code_provider
    scale::scale
    logger
    log_configurator
    module_repository
    )

addtest(runtime_upgrade_tracker_test
    runtime_upgrade_tracker_test.cpp
    )
target_link_libraries(runtime_upgrade_tracker_test
    runtime_upgrade_tracker
    blob
    storage
    logger
    log_configurator
    scale::scale
    )

addtest(instance_pool_test instance_pool_test.cpp)
target_link_libraries(instance_pool_test
    module_repository
    blob
    logger
    log_configurator
    )

addtest(stack_limiter_test stack_limiter_test.cpp)
target_link_libraries(stack_limiter_test
    logger
    log_configurator
    wasm_instrument
    )

addtest(sqlite_test sqlite_test.cpp)
target_link_libraries(sqlite_test
    SQLiteModernCpp::SQLiteModernCpp
)