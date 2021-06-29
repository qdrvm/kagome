

addtest(wasm_memory_test
    wasm_memory_test.cpp
    )
target_link_libraries(wasm_memory_test
    wavm_memory
    logger_for_tests
    )

addtest(core_integration_test
    core_integration_test.cpp
    )
target_link_libraries(core_integration_test
    runtime_test
    wavm_core_api
    basic_wasm_provider
    logger_for_tests
    )

addtest(wasm_executor_test
    wasm_executor_test.cpp
    )
target_link_libraries(wasm_executor_test
    wavm_executor
    basic_wasm_provider
    wavm_core_api
    trie_storage
    trie_storage_backend
    trie_storage_provider
    in_memory_storage
    polkadot_trie_factory
    trie_serializer
    logger_for_tests
    )
