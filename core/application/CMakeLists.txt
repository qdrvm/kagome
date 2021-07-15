add_library(application_util
    impl/util.cpp
    )
target_link_libraries(application_util
    Boost::filesystem
    )

add_library(app_config_impl
    impl/app_configuration_impl.cpp
    )
target_link_libraries(app_config_impl
    Boost::program_options
    p2p::p2p_multiaddress
    blob
    assets
    configuration_storage
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
    )

add_library(app_state_manager
    impl/app_state_manager_impl.cpp
    )
target_link_libraries(app_state_manager
    logger
    )

add_library(kagome_application
    impl/kagome_application_impl.cpp
    )
target_link_libraries(kagome_application
    application_injector
    application_util
    log_configurator
   )
