
addtest(json_configuration_reader_test
    json_configuration_reader_test.cpp
    )
target_link_libraries(json_configuration_reader_test
    json_configuration_reader
    Boost::filesystem
    scale
    primitives
    )

addtest(configuration_storage_test
    configuration_storage_test.cpp
    )
target_link_libraries(configuration_storage_test
    configuration_storage
    primitives
    )

addtest(local_key_storage_test
    local_key_storage_test.cpp
    )
target_link_libraries(local_key_storage_test
    key_storage
    Boost::filesystem
    Boost::random
    p2p::p2p_key_generator
    p2p::p2p_key_validator
    )
