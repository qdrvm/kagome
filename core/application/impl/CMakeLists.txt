add_library(config_reader_error
    config_reader/error.cpp
    config_reader/error.hpp
    )
target_link_libraries(config_reader_error
    outcome
    )

add_library(pt_util
    config_reader/pt_util.hpp
    config_reader/pt_util.cpp
    )
target_link_libraries(pt_util
    Boost::boost
    config_reader_error
    )

add_library(json_configuration_reader
    config_reader/json_configuration_reader.hpp
    config_reader/json_configuration_reader.cpp
    )
target_link_libraries(json_configuration_reader
    pt_util
    config_reader_error
    )
