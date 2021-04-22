
add_library(wavm_executor
    impl/module.cpp
    impl/module_instance.cpp
    impl/module_repository.cpp
    impl/intrinsic_resolver.cpp
    )
target_link_libraries(wavm_executor
    libWAVM
    Boost::boost
    )

add_library(wavm_account_nonce_api runtime_api/account_nonce_api.hpp)
target_link_libraries(wavm_account_nonce_api wavm_executor)
add_library(wavm_babe_api runtime_api/babe_api.hpp)
target_link_libraries(wavm_babe_api wavm_executor)
add_library(wavm_block_builder_api runtime_api/block_builder.hpp)
target_link_libraries(wavm_block_builder_api wavm_executor)
add_library(wavm_core_api runtime_api/core.hpp)
target_link_libraries(wavm_core_api wavm_executor)
add_library(wavm_grandpa_api runtime_api/grandpa_api.hpp)
target_link_libraries(wavm_grandpa_api wavm_executor)
add_library(wavm_metadata_api runtime_api/metadata.hpp)
target_link_libraries(wavm_metadata_api wavm_executor)
add_library(wavm_offchain_worker_api runtime_api/offchain_worker.hpp)
target_link_libraries(wavm_offchain_worker_api wavm_executor)
add_library(wavm_parachain_host_api runtime_api/parachain_host.hpp)
target_link_libraries(wavm_parachain_host_api wavm_executor)
add_library(wavm_tagged_transaction_queue_api runtime_api/tagged_transaction_queue.hpp)
target_link_libraries(wavm_tagged_transaction_queue_api wavm_executor)
add_library(wavm_transaction_payment_api runtime_api/transaction_payment_api.hpp)
target_link_libraries(wavm_transaction_payment_api wavm_executor)
