#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(consensus_client_libp2p
    consensus_client_libp2p.cpp
    consensus_client_libp2p.hpp
    )
target_link_libraries(consensus_client_libp2p
    outcome
    buffer
    scale
    logger
    rpc_sender_libp2p
    )

add_library(consensus_server_libp2p
    consensus_server_libp2p.cpp
    consensus_server_libp2p.hpp
    )
target_link_libraries(consensus_server_libp2p
    outcome
    scale
    logger
    scale_rpc_receiver
    )

add_library(gossiper_client_libp2p
    gossiper_client_libp2p.cpp
    gossiper_client_libp2p.hpp
    )
target_link_libraries(gossiper_client_libp2p
    outcome
    buffer
    scale
    logger
    rpc_sender_libp2p
    )

add_library(gossiper_server_libp2p
    gossiper_server_libp2p.cpp
    gossiper_server_libp2p.hpp
    )
target_link_libraries(gossiper_server_libp2p
    outcome
    scale
    logger
    scale_rpc_receiver
    )

add_library(gossiper_libp2p
    gossiper_broadcast.cpp
    gossiper_broadcast.hpp
    )
target_link_libraries(gossiper_libp2p
    gossiper_client_libp2p
    )

add_library(rpc_sender_libp2p
    rpc_sender_libp2p.cpp
    rpc_sender_libp2p.hpp
    )
target_link_libraries(rpc_sender_libp2p
    message_read_writer
    outcome
    )

add_library(scale_rpc_receiver
    scale_rpc_receiver.cpp
    scale_rpc_receiver.hpp
    )
target_link_libraries(scale_rpc_receiver
    message_read_writer
    outcome
    scale
    )
