add_library(logger
    logger.cpp
    )
target_link_libraries(logger
    fmt::fmt
    soralog::soralog
    p2p::p2p_logger
    )
kagome_install(logger)

add_library(log_configurator
    configurator.cpp
    )
target_link_libraries(log_configurator
    soralog::configurator_yaml
    )
kagome_install(log_configurator)
