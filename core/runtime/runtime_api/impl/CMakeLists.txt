
add_library(core_api
    core.cpp)
target_link_libraries(core_api
    executor
    primitives
    )
kagome_install(core_api)

add_library(account_nonce_api
    account_nonce_api.cpp
    )
target_link_libraries(account_nonce_api
    executor
    )

add_library(authority_discovery_api
    authority_discovery_api.cpp)
target_link_libraries(authority_discovery_api
    executor
    )

add_library(babe_api
    babe_api.cpp
    )
target_link_libraries(babe_api
    executor
    )

add_library(beefy_api
    beefy.cpp
    )
target_link_libraries(beefy_api
    executor
    runtime_transaction_error
    )

add_library(block_builder_api
    block_builder.cpp
    )
target_link_libraries(block_builder_api
    executor primitives
    )

add_library(grandpa_api
    grandpa_api.cpp
    )
target_link_libraries(grandpa_api
    blockchain
    executor
    )

add_library(metadata_api
    metadata.cpp
    )
target_link_libraries(metadata_api
    executor
    )
kagome_install(metadata_api)

add_library(parachain_host_api
    parachain_host.cpp
    parachain_host_types_serde.cpp
    )
target_link_libraries(parachain_host_api
    executor
    )

add_library(tagged_transaction_queue_api
    tagged_transaction_queue.cpp
    )
target_link_libraries(tagged_transaction_queue_api
    executor
    )

add_library(transaction_payment_api
    transaction_payment_api.cpp
    )
target_link_libraries(transaction_payment_api
    executor
    )

add_library(offchain_worker_api
    offchain_worker_api.cpp
    )
target_link_libraries(offchain_worker_api
    offchain_worker
    )

add_library(session_keys_api
    session_keys_api.cpp
    )
target_link_libraries(session_keys_api
    executor
    )

add_library(runtime_properties_cache
    ../../common/runtime_properties_cache_impl.cpp
    )
target_link_libraries(runtime_properties_cache
    executor
    )
kagome_install(runtime_properties_cache)



