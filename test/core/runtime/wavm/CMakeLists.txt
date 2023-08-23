add_library(wavm_runtime_test INTERFACE)
target_link_libraries(wavm_runtime_test INTERFACE
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
    runtime_wavm
    module_repository
    runtime_upgrade_tracker
    )


addtest(wavm_memory_test
    wasm_memory_test.cpp
    )
target_link_libraries(wavm_memory_test
    wavm_memory
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

addtest(wasm_executor_test
    wasm_executor_test.cpp
    )
target_link_libraries(wasm_executor_test
    runtime_wavm
    basic_code_provider
    core_api
    trie_storage_provider
    host_api_factory
    crypto_store
    key_file_storage
    sr25519_provider
    ecdsa_provider
    ed25519_provider
    pbkdf2_provider
    hasher
    primitives
    storage
    logger_for_tests
    module_repository
    )

addtest(wavm_module_init_test
    wavm_module_init_test.cpp
    )
target_link_libraries(wavm_module_init_test
    runtime_wavm
    basic_code_provider
    )