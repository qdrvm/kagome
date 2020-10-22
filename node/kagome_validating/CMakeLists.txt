#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(kagome_validating
    main.cpp
    )
target_link_libraries(kagome_validating
    Boost::boost
    Boost::program_options
    validating_node_application
    app_config_impl
    p2p::p2p_identify
    p2p::p2p_ping
    )
kagome_install(kagome_validating)
