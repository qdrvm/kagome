add_library(runtime_test INTERFACE)
target_link_libraries(runtime_test INTERFACE key_file_storage sr25519_provider ed25519_provider pbkdf2_provider bip39_provider secp256k1_provider hasher crypto_store host_api host_api_factory binaryen_core_api_provider)

addtest(tagged_transaction_queue_test
    tagged_transaction_queue_test.cpp
    )
target_link_libraries(tagged_transaction_queue_test
    runtime_test
    binaryen_tagged_transaction_queue_api
    testutil_primitives_generator
    basic_wasm_provider
    binaryen_core_api
    binaryen_wasm_memory_factory
    logger_for_tests
    )

addtest(block_builder_api_test
    block_builder_api_test.cpp
    )
target_link_libraries(block_builder_api_test
    runtime_test
    binaryen_block_builder_api
    basic_wasm_provider
    binaryen_core_api
    binaryen_wasm_memory_factory
    logger_for_tests
    )

addtest(parachain_test
    parachain_test.cpp
    )
target_link_libraries(parachain_test
    runtime_test
    binaryen_parachain_host_api
    basic_wasm_provider
    binaryen_core_api
    binaryen_wasm_memory_factory
    hasher
    pbkdf2_provider
    )

addtest(grandpa_api_test
    grandpa_api_test.cpp
    )
target_link_libraries(grandpa_api_test
    runtime_test
    binaryen_grandpa_api
    trie_storage_backend
    basic_wasm_provider
    key_file_storage
    binaryen_wasm_memory_factory
    binaryen_core_api
    logger_for_tests
    )

addtest(metadata_test
    metadata_test.cpp
    )
target_link_libraries(metadata_test
    runtime_test
    binaryen_metadata_api
    basic_wasm_provider
    trie_storage
    binaryen_wasm_memory_factory
    binaryen_core_api
    logger_for_tests
    )

addtest(offchain_worker_test
    offchain_worker_test.cpp
    )
target_link_libraries(offchain_worker_test
    runtime_test
    binaryen_offchain_worker_api
    basic_wasm_provider
    trie_storage_backend
    binaryen_wasm_memory_factory
    binaryen_core_api
    logger_for_tests
    )
# TODO(Harrm): actualize this
#addtest(runtime_external_interface_test
#    runtime_external_interface_test.cpp
#    )
#target_link_libraries(runtime_external_interface_test
#    binaryen_runtime_external_interface
#    binaryen_wasm_memory_factory
#    blob
#    logger_for_tests
#    )
