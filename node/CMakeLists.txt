#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(kagome
    main.cpp
    )
target_link_libraries(kagome
    Boost::boost
    Boost::program_options
    kagome_application
    app_config
    fd_limit
    p2p::p2p_identify
    p2p::p2p_ping
    )
if (BACKWARD)
  add_backward(kagome)
endif()
kagome_install(kagome)
