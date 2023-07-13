add_library(application_util
    impl/util.cpp
    )
target_link_libraries(application_util
    filesystem
    outcome
    )

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
    COMMAND echo "#include <string>" > ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
    COMMAND echo "namespace kagome {" >> ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
    COMMAND echo "  const std::string &buildVersion() {" >> ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
    COMMAND echo "    static const std::string buildVersion(\"" | tr -d "\\n" >> ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
    COMMAND ${PROJECT_SOURCE_DIR}/get_version.sh | tr -d "\\n" >> ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
    COMMAND echo "\");" >> ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
    COMMAND echo "    return buildVersion;" >> ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
    COMMAND echo "  }" >> ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
    COMMAND echo "}" >> ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
    COMMENT "Generate build_version.cpp"
    VERBATIM
    )
add_library(build_version
    ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
    )
kagome_install(build_version)

add_library(app_config
    impl/app_configuration_impl.cpp
    )
target_link_libraries(app_config
    Boost::program_options
    p2p::p2p_multiaddress
    api
    blob
    assets
    chain_spec
    build_version
    )
kagome_install(app_config)

add_library(chain_spec
    impl/chain_spec_impl.cpp
    )
target_link_libraries(chain_spec
    assets
    filesystem
    p2p::p2p_multiaddress
    p2p::p2p_peer_id
    sr25519_types
    )
kagome_install(chain_spec)

add_library(app_state_manager
    impl/app_state_manager_impl.cpp
    )
target_link_libraries(app_state_manager
    logger
    )
kagome_install(app_state_manager)

add_library(application_modes
    modes/print_chain_info_mode.cpp
    )
target_link_libraries(application_modes
    hexutil
    RapidJSON::rapidjson
    benchmark::benchmark
    )

add_library(recovery_mode
    modes/recovery_mode.cpp
    )
target_link_libraries(recovery_mode
    logger
    )

add_library(kagome_application
    impl/kagome_application_impl.cpp
    )
target_link_libraries(kagome_application
    application_injector
    application_util
    log_configurator
    telemetry
   )
