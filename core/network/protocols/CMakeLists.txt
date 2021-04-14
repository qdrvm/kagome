#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(block_announce_protocol
    block_announce_protocol.cpp
    )
target_link_libraries(block_announce_protocol
    logger
    scale_message_read_writer
    )

add_library(gossip_protocol
    gossip_protocol.cpp
    )
target_link_libraries(gossip_protocol
    logger
    p2p::p2p_loopback_stream
    )

add_library(grandpa_protocol
    grandpa_protocol.cpp
    )
target_link_libraries(grandpa_protocol
    logger
    )

add_library(propagate_transactions_protocol
    propagate_transactions_protocol.cpp
    )
target_link_libraries(propagate_transactions_protocol
    logger
    )

add_library(sup_protocol
    sup_protocol.cpp
    )
target_link_libraries(sup_protocol
    logger
    )

add_library(sync_protocol
    sync_protocol.cpp
    )
target_link_libraries(sync_protocol
    logger
    node_api_proto
    adapter_errors
    )

add_library(protocol_factory
    protocol_factory.cpp
    )
target_link_libraries(protocol_factory
    logger
    grandpa_protocol
    sup_protocol
    sync_protocol
    )

