
addtest(json_configuration_reader_test
    json_configuration_reader_test.cpp
    )
target_link_libraries(json_configuration_reader_test
    json_configuration_reader
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
