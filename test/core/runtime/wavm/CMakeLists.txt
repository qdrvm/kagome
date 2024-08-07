#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(wavm_runtime_test INTERFACE)
target_link_libraries(wavm_runtime_test INTERFACE
    sr25519_provider
    ecdsa_provider
    ed25519_provider
    pbkdf2_provider
    bip39_provider
    secp256k1_provider
    hasher
    key_store
    host_api
    host_api_factory
    core_api
    core_api_factory
    runtime_wavm
    module_repository
    runtime_upgrade_tracker
    )


addtest(wavm_memory_test
    wasm_memory_test.cpp
    )
target_link_libraries(wavm_memory_test
    logger_for_tests
    runtime_wavm
    )

addtest(core_integration_test
    core_integration_test.cpp
    )
target_link_libraries(core_integration_test
    wavm_runtime_test
    core_api
    basic_code_provider
    storage
    filesystem
    logger_for_tests
    )
