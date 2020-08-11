#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(kagome_block_producing
    main.cpp
    )
target_link_libraries(kagome_block_producing
    block_producing_node_application
    Boost::program_options
    app_config_impl
    )
kagome_install(kagome_block_producing)
