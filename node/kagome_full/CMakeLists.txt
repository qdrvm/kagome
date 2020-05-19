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
    )

add_executable(kagome_full
    main.cpp
    )
target_link_libraries(kagome_full
    Boost::boost
    application_injector
    block_storage
    kagome_application
    kagome_options
    local_key_storage
    primitives
    )
