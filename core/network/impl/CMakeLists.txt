#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(gossiper_broadcast
    gossiper_broadcast.cpp
    gossiper_broadcast.hpp
    )
target_link_libraries(gossiper_broadcast
    scale_message_read_writer
    )

add_library(kagome_router
    router_libp2p.cpp
    router_libp2p.hpp
    )
target_link_libraries(kagome_router
    logger
    scale
    )
