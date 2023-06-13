#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(kagome SHARED
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
    kagome-benchmark
    kagome-db-editor
    storage_explorer
    filesystem
    )
if (BACKWARD)
  add_backward(kagome)
endif()

set_target_properties(kagome PROPERTIES PREFIX "" DEBUG_POSTFIX "")
add_executable(kagome_dlopen dlopen.c)
set_target_properties(kagome_dlopen PROPERTIES OUTPUT_NAME kagome)
