#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(kagome_configuration)

addtest(json_configuration_reader_test
    json_configuration_reader_test.cpp
    )
target_link_libraries(json_configuration_reader_test
    json_configuration_reader
    example_config
    scale
    primitives
    )

addtest(configuration_storage_test
    configuration_storage_test.cpp
    )
target_link_libraries(configuration_storage_test
    configuration_storage
    )

addtest(local_key_storage_test
    local_key_storage_test.cpp
    )
target_link_libraries(local_key_storage_test
    local_key_storage
    Boost::filesystem
    )
