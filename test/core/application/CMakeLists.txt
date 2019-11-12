
addtest(json_configuration_storage_test
    json_configuration_storage_test.cpp
    )
target_link_libraries(json_configuration_storage_test
    json_configuration_reader
    configuration_storage
    Boost::filesystem
    )
