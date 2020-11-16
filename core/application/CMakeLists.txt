add_library(config_reader_error
    impl/config_reader/error.cpp
    impl/config_reader/error.hpp
    )
target_link_libraries(config_reader_error
    outcome
    )

add_library(application_util
    impl/util.cpp
    )
target_link_libraries(application_util
    Boost::filesystem
    )

add_library(validating_node_application
    impl/validating_node_application.cpp
    )
target_link_libraries(validating_node_application
    validating_node_injector
    blob
    application_util
    )

add_library(app_config_impl
    impl/app_configuration_impl.cpp
    impl/util.hpp impl/util.cpp)
target_link_libraries(app_config_impl
    Boost::program_options
    )

add_library(syncing_node_application
    impl/syncing_node_application.cpp
    )
target_link_libraries(syncing_node_application
    syncing_node_injector
    blob
    application_util
    )

add_library(block_producing_node_application
    impl/block_producing_node_application.cpp
    )
target_link_libraries(block_producing_node_application
    block_producing_node_injector
    blob
    application_util
    )

add_library(configuration_storage
    impl/chain_spec_impl.cpp
    )
target_link_libraries(configuration_storage
    Boost::filesystem
    p2p::p2p_multiaddress
    p2p::p2p_peer_id
    buffer
    sr25519_types
    config_reader_error
    )

add_library(app_state_manager
    impl/app_state_manager_impl.cpp
    )
target_link_libraries(app_state_manager
    logger
    )
