add_subdirectory(impl)

add_library(configuration_storage
    impl/configuration_storage_impl.cpp
    )
target_link_libraries(configuration_storage
    Boost::filesystem
    config_reader_error
    p2p::p2p_multiaddress
    p2p::p2p_peer_id
    buffer
    sr25519_types
    )

add_library(local_key_storage
    impl/local_key_storage.cpp
    )
target_link_libraries(local_key_storage
    primitives
    config_reader_error
    )
