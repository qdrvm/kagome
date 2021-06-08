

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
    binaryen_core_api
    basic_wasm_provider
    binaryen_wasm_memory_factory
    logger_for_tests
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
    binaryen_wasm_memory_factory
    polkadot_trie_factory
    trie_serializer
    logger_for_tests
    )
