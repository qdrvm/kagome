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
    )

addtest(block_builder_api_test
    block_builder_api_test.cpp
    )
target_link_libraries(block_builder_api_test
    binaryen_block_builder_api
    basic_wasm_provider
    extension_factory
    )

addtest(wasm_executor_test
    wasm_executor_test.cpp
    )
target_link_libraries(wasm_executor_test
    binaryen_wasm_executor
    basic_wasm_provider
    )

addtest(parachain_test
    parachain_test.cpp
    )
target_link_libraries(parachain_test
    binaryen_parachain_host_api
    basic_wasm_provider
    extension_factory
    )

addtest(grandpa_test
    grandpa_test.cpp
    )
target_link_libraries(grandpa_test
    binaryen_grandpa_api
    persistent_trie_db_backend
    basic_wasm_provider
    extension_factory
    )

addtest(metadata_test
    metadata_test.cpp
    )
target_link_libraries(metadata_test
    binaryen_metadata_api
    basic_wasm_provider
    extension_factory
    )

addtest(offchain_worker_test
    offchain_worker_test.cpp
    )
target_link_libraries(offchain_worker_test
    binaryen_offchain_worker_api
    basic_wasm_provider
    persistent_trie_db_backend
    extension_factory
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
