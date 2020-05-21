add_library(config_reader_error
    impl/config_reader/error.cpp
    impl/config_reader/error.hpp
    )
target_link_libraries(config_reader_error
    outcome
    )

add_library(validating_node_application
    impl/validating_node_application.cpp
    )
target_link_libraries(validating_node_application
    validating_node_injector
    blob
    )

add_library(syncing_node_application
    impl/syncing_node_application.cpp
    )
target_link_libraries(syncing_node_application
    syncing_node_injector
    blob
    )

add_library(block_producing_node_application
    impl/block_producing_node_application.cpp
    )
target_link_libraries(block_producing_node_application
    block_producing_node_injector
    blob
    )

add_library(configuration_storage
    impl/configuration_storage_impl.cpp
    )
target_link_libraries(configuration_storage
    Boost::filesystem
    p2p::p2p_multiaddress
    p2p::p2p_peer_id
    buffer
    sr25519_types
    config_reader_error
    )

add_library(local_key_storage
    impl/local_key_storage.cpp
    )
target_link_libraries(local_key_storage
    primitives
    config_reader_error
    Boost::filesystem
    sr25519_types
    ed25519_types
    p2p::p2p_crypto_key
    )
