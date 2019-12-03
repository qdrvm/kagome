add_library(config_reader_error
    impl/config_reader/error.cpp
    impl/config_reader/error.hpp
    )
target_link_libraries(config_reader_error
    outcome
    )

add_library(pt_util
    impl/config_reader/pt_util.cpp
    )
target_link_libraries(pt_util
    hexutil
    config_reader_error
    )

add_library(kagome_application
    impl/kagome_application_impl.cpp
    )
target_link_libraries(kagome_application
    application_injector
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
    pt_util
    )

add_library(local_key_storage
    impl/local_key_storage.cpp
    )
target_link_libraries(local_key_storage
    primitives
    pt_util
    Boost::filesystem
    sr25519_types
    ed25519_types
    p2p::p2p_crypto_key
    )
