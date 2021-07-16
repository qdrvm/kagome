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
    )

add_library(sync_protocol
    sync_protocol.cpp
    )
target_link_libraries(sync_protocol
    logger
    node_api_proto
    adapter_errors
    protocol_error
    )

add_library(protocol_factory
    protocol_factory.cpp
    )
target_link_libraries(protocol_factory
    logger
    )
