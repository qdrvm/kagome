#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(binaryen_runtime_test INTERFACE)
target_link_libraries(binaryen_runtime_test INTERFACE
    filesystem
    key_file_storage
    sr25519_provider
    ecdsa_provider
    ed25519_provider
    pbkdf2_provider
    bip39_provider
    secp256k1_provider
    hasher
    crypto_store
    host_api
    host_api_factory
    core_api
    binaryen_executor_factory
    binaryen_module_factory
    binaryen_wasm_memory_factory
    module_repository
    runtime_upgrade_tracker
    storage
    )

addtest(tagged_transaction_queue_test
    tagged_transaction_queue_test.cpp
    )
target_link_libraries(tagged_transaction_queue_test
    binaryen_runtime_test
    tagged_transaction_queue_api
    testutil_primitives_generator
    basic_code_provider
    core_api
    logger_for_tests
    filesystem
    )

addtest(block_builder_api_test
    block_builder_api_test.cpp
    )
target_link_libraries(block_builder_api_test
    binaryen_runtime_test
    block_builder_api
    basic_code_provider
    core_api
    filesystem
    logger_for_tests
    )

addtest(parachain_test
    parachain_test.cpp
    )
target_link_libraries(parachain_test
    binaryen_runtime_test
    parachain_host_api
    basic_code_provider
    core_api
    filesystem
    hasher
    pbkdf2_provider
    network
    )

addtest(metadata_test
    metadata_test.cpp
    )
target_link_libraries(metadata_test
    binaryen_runtime_test
    metadata_api
    basic_code_provider
    storage
    core_api
    logger_for_tests
    )

addtest(runtime_external_interface_test
    runtime_external_interface_test.cpp
    )
target_link_libraries(runtime_external_interface_test
    binaryen_runtime_external_interface
    executor
    constant_code_provider
    blob
    logger_for_tests
    )

addtest(binaryen_memory_test
    wasm_memory_test.cpp
    )
target_link_libraries(binaryen_memory_test
    binaryen_wasm_memory
    binaryen_runtime_external_interface
    log_configurator
    hexutil
    )
