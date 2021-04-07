#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(block_announce_protocol
    block_announce_protocol.cpp
    )
target_link_libraries(block_announce_protocol
    logger
    )

add_library(sync_protocol
    sync_protocol.cpp
    )
target_link_libraries(sync_protocol
    logger
    node_api_proto
    )

add_library(sup_protocol
    sup_protocol.cpp
    )
target_link_libraries(sup_protocol
    logger
    )

add_library(gossip_protocol
    gossip_protocol.cpp
    )
target_link_libraries(gossip_protocol
    logger
    loopback_stream
    )

add_library(propagate_transactions_protocol
    propagate_transactions_protocol.cpp
    )
target_link_libraries(propagate_transactions_protocol
    logger
    )

