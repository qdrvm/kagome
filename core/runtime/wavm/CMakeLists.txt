add_library(compartment_wrapper impl/compartment_wrapper.cpp)
target_link_libraries(compartment_wrapper libWAVM buffer)
kagome_install(compartment_wrapper)

add_library(wavm_memory impl/memory.cpp)
target_link_libraries(wavm_memory libWAVM buffer)
kagome_install(wavm_memory)

add_library(wavm_executor
    executor.cpp
    impl/module.cpp
    impl/module_instance.cpp
    impl/module_repository_impl.cpp
    impl/intrinsic_resolver_impl.cpp
    impl/core_api_factory.cpp
    impl/intrinsic_functions.cpp
    impl/wavm_memory_provider.cpp
    impl/intrinsic_module_instance.cpp
    )
target_link_libraries(wavm_executor
    libWAVM
    wavm_memory
    constant_code_provider
    Boost::boost
    compartment_wrapper
    )
kagome_install(wavm_executor)

add_library(wavm_account_nonce_api runtime_api/account_nonce_api.cpp)
target_link_libraries(wavm_account_nonce_api wavm_executor)
add_library(wavm_babe_api runtime_api/babe_api.cpp)
target_link_libraries(wavm_babe_api wavm_executor)
add_library(wavm_block_builder_api runtime_api/block_builder.cpp)
target_link_libraries(wavm_block_builder_api wavm_executor)
add_library(wavm_core_api runtime_api/core.cpp)
target_link_libraries(wavm_core_api wavm_executor)
add_library(wavm_grandpa_api runtime_api/grandpa_api.cpp)
target_link_libraries(wavm_grandpa_api wavm_executor)
add_library(wavm_metadata_api runtime_api/metadata.cpp)
target_link_libraries(wavm_metadata_api wavm_executor)
add_library(wavm_offchain_worker_api runtime_api/offchain_worker.cpp)
target_link_libraries(wavm_offchain_worker_api wavm_executor)
add_library(wavm_parachain_host_api runtime_api/parachain_host.cpp)
target_link_libraries(wavm_parachain_host_api wavm_executor)
add_library(wavm_tagged_transaction_queue_api runtime_api/tagged_transaction_queue.cpp)
target_link_libraries(wavm_tagged_transaction_queue_api wavm_executor)
add_library(wavm_transaction_payment_api runtime_api/transaction_payment_api.cpp)
target_link_libraries(wavm_transaction_payment_api wavm_executor)
