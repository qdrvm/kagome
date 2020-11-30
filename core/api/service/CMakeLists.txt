#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_service
    api_service.hpp
    api_service.cpp
    )
target_link_libraries(api_service
    Boost::boost
    logger
    app_state_manager
    rpc_thread_pool
    p2p::p2p_peer_id
    )

add_subdirectory(author)
add_subdirectory(chain)
add_subdirectory(state)
add_subdirectory(system)
add_subdirectory(rpc)
