#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(config_reader_error
    config_reader/error.cpp
    config_reader/error.hpp
    )
target_link_libraries(config_reader_error
    outcome
    )

add_library(kagome_application
    kagome_application_impl.cpp
    )
target_link_libraries(kagome_application
    application_injector
    blob
    )
