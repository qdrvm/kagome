#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

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

add_subdirectory(binaryen)

# TODO(Harrm): Remake tests for WAVM
add_subdirectory(wavm)
