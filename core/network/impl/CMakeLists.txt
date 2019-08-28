#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(peer_client
    peer_client_libp2p.cpp
    peer_client_libp2p.hpp
    )
target_link_libraries(peer_client
    outcome
    buffer
    scale
    logger
    libp2p_peer_id
    multiaddress
    rpc_sender_libp2p
    )

add_library(peer_server
    peer_server_libp2p.cpp
    peer_server_libp2p.hpp
    )
target_link_libraries(peer_server
    outcome
    buffer
    scale
    logger
    message_read_writer
    libp2p_peer_id
    multiaddress
    )

add_library(rpc_sender_libp2p
    rpc_sender_libp2p.cpp
    rpc_sender_libp2p.hpp
    )
target_link_libraries(rpc_sender_libp2p
    message_read_writer
    outcome
    )
