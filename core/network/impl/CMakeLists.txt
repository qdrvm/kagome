#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(grandpa_transmitter
    grandpa_transmitter_impl.cpp
    )
target_link_libraries(grandpa_transmitter
    logger
    )

add_library(block_announce_transmitter
    block_announce_transmitter_impl.cpp
    )
target_link_libraries(block_announce_transmitter
    logger
    )

add_library(transactions_transmitter
    transactions_transmitter_impl.cpp
    )
target_link_libraries(transactions_transmitter
    logger
    )

add_library(kagome_router
    router_libp2p.cpp
    router_libp2p.hpp
    )
target_link_libraries(kagome_router
    logger
    outcome
    block_announce_protocol
    )

add_library(extrinsic_observer
    extrinsic_observer_impl.hpp
    extrinsic_observer_impl.cpp
    )
target_link_libraries(extrinsic_observer
    logger
    )

add_library(sync_protocol_observer
    sync_protocol_observer_impl.hpp
    sync_protocol_observer_impl.cpp
    )
target_link_libraries(sync_protocol_observer
    block_header_repository
    logger
    p2p::p2p_peer_id
    )

add_library(kademlia_storage_backend
    kademlia_storage_backend.cpp
    )
target_link_libraries(kademlia_storage_backend
    logger
    )

add_library(peer_manager
    peer_manager_impl.cpp
    )
target_link_libraries(peer_manager
    logger
    )
