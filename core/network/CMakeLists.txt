#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(protobuf)


add_library(network
    impl/state_protocol_observer_impl.cpp
    impl/synchronizer_impl.cpp
    impl/router_libp2p.cpp
    impl/grandpa_transmitter_impl.cpp
    impl/block_announce_transmitter_impl.cpp
    impl/extrinsic_observer_impl.cpp
    impl/transactions_transmitter_impl.cpp
    impl/sync_protocol_observer_impl.cpp
    impl/protocols/protocol_error.cpp
    impl/protocols/state_protocol_impl.cpp
    impl/protocols/protocol_factory.cpp
    impl/protocols/sync_protocol_impl.cpp
    impl/protocols/propagate_transactions_protocol.cpp
    impl/protocols/grandpa_protocol.cpp
    impl/protocols/protocol_req_collation.cpp
    impl/protocols/collation_protocol.cpp
    impl/protocols/block_announce_protocol.cpp
    impl/peer_view.cpp
    impl/peer_manager_impl.cpp
    impl/reputation_repository_impl.cpp
    helpers/scale_message_read_writer.cpp
    adapters/adapter_errors.cpp
    )
target_link_libraries(network
    p2p::p2p_basic_scheduler
    scale_libp2p_types
    blockchain
    logger
    p2p::p2p_peer_id
    primitives
    metrics
    telemetry
    p2p::p2p_message_read_writer
    scale::scale
    outcome
    blob
    node_api_proto
    p2p::p2p_loopback_stream
    )
add_custom_command(TARGET network
    POST_BUILD
    COMMAND find ${CMAKE_CURRENT_BINARY_DIR} -name "*.o" -type f -delete
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Removing object files from '${CMAKE_CURRENT_BINARY_DIR}'"
)
