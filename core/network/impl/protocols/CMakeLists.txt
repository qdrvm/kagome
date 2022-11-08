#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(protocol_error
    protocol_error.cpp
    )
target_link_libraries(protocol_error
    outcome
    )

add_library(block_announce_protocol
    block_announce_protocol.cpp
    )
target_link_libraries(block_announce_protocol
    logger
    scale_message_read_writer
    protocol_error
    )

add_library(grandpa_protocol
    grandpa_protocol.cpp
    )
target_link_libraries(grandpa_protocol
    logger
    protocol_error
    )

add_library(propagate_transactions_protocol
    propagate_transactions_protocol.cpp
    )
target_link_libraries(propagate_transactions_protocol
    logger
    protocol_error
    metrics
    )


add_library(state_protocol
    state_protocol_impl.cpp
    )
target_link_libraries(state_protocol
    logger
    node_api_proto
    protocol_error
    )

add_library(sync_protocol
    sync_protocol_impl.cpp
    )
target_link_libraries(sync_protocol
    blob
    logger
    node_api_proto
    adapter_errors
    protocol_error
    scale_message_read_writer
    )

add_library(req_collation_protocol
    protocol_req_collation.cpp
    )
target_link_libraries(req_collation_protocol
    blob
    logger
    adapter_errors
    protocol_error
    scale_message_read_writer
    p2p::p2p_peer_id
    )

add_library(protocol_factory
    protocol_factory.cpp
    )
target_link_libraries(protocol_factory
    state_protocol
    sync_protocol
    logger
    )
