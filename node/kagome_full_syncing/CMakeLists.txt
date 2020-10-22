#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(kagome_full_syncing
    main.cpp
    )
target_link_libraries(kagome_full_syncing
    syncing_node_application
    Boost::program_options
    app_config_impl
    p2p::p2p_identify
    p2p::p2p_ping
    )
kagome_install(kagome_full_syncing)
