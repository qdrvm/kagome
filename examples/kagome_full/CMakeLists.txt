#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(kagome_options
    kagome_options.cpp
    )
target_link_libraries(kagome_options
    Boost::boost
    Boost::program_options
    example_config
    json_configuration_reader
    )

add_executable(kagome-launch
    main.cpp
    )
target_link_libraries(kagome-launch
    Boost::boost
    application_injector
    block_storage
    kagome_application
    kagome_options
    key_storage
    primitives
    )
