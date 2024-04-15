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

addtest(wasm_result_test
    wasm_result_test.cpp
    )

addtest(storage_code_provider_test
    storage_code_provider_test.cpp
    )
target_link_libraries(storage_code_provider_test
    logger_for_tests
    )

addtest(trie_storage_provider_test
    trie_storage_provider_test.cpp
    )
target_link_libraries(trie_storage_provider_test
    logger_for_tests
    )

addtest(uncompress_code_test
    uncompress_code_test.cpp
    )

addtest(executor_test
    executor_test.cpp
    )
target_link_libraries(executor_test
    basic_code_provider
    )

addtest(runtime_upgrade_tracker_test
    runtime_upgrade_tracker_test.cpp
    )

addtest(instance_pool_test instance_pool_test.cpp)

addtest(stack_limiter_test stack_limiter_test.cpp)
