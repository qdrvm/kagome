#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(wasm_memory_test
    wasm_memory_test.cpp
    )
target_link_libraries(wasm_memory_test
    binaryen_wasm_memory
    )

addtest(runtime_external_interface_test
    runtime_external_interface_test.cpp
    )
target_link_libraries(runtime_external_interface_test
    binaryen_runtime_external_interface
    )

addtest(core_integration_test
    core_integration_test.cpp
    )
target_link_libraries(core_integration_test
    binaryen_core_api
    basic_wasm_provider
    extension_factory
    )

addtest(tagged_transaction_queue_test
    tagged_transaction_queue_test.cpp
    )
target_link_libraries(tagged_transaction_queue_test
    binaryen_tagged_transaction_queue_api
    testutil_primitives_generator
    basic_wasm_provider
    extension_factory
    binaryen_core_api
    binaryen_core_api
    )

addtest(block_builder_api_test
    block_builder_api_test.cpp
    )
target_link_libraries(block_builder_api_test
    binaryen_block_builder_api
    basic_wasm_provider
    extension_factory
    binaryen_core_api
    )

addtest(wasm_executor_test
    wasm_executor_test.cpp
    )
target_link_libraries(wasm_executor_test
    binaryen_wasm_executor
    basic_wasm_provider
    binaryen_core_api
    trie_storage
    trie_storage_backend
    trie_storage_provider
    in_memory_storage
    extension_factory
    polkadot_trie_factory
    trie_serializer
    )

addtest(parachain_test
    parachain_test.cpp
    )
target_link_libraries(parachain_test
    binaryen_parachain_host_api
    basic_wasm_provider
    extension_factory
    binaryen_core_api
    )

addtest(grandpa_api_test
    grandpa_api_test.cpp
    )
target_link_libraries(grandpa_api_test
    binaryen_grandpa_api
    trie_storage_backend
    basic_wasm_provider
    extension_factory
    binaryen_core_api
    )

addtest(metadata_test
    metadata_test.cpp
    )
target_link_libraries(metadata_test
    binaryen_metadata_api
    basic_wasm_provider
    extension_factory
    trie_storage
    binaryen_core_api
    )

addtest(offchain_worker_test
    offchain_worker_test.cpp
    )
target_link_libraries(offchain_worker_test
    binaryen_offchain_worker_api
    basic_wasm_provider
    trie_storage_backend
    extension_factory
    binaryen_core_api
    )

addtest(wasm_result_test
    wasm_result_test.cpp
    )

addtest(storage_wasm_provider_test
    storage_wasm_provider_test.cpp
    )
target_link_libraries(storage_wasm_provider_test
    storage_wasm_provider
    )
