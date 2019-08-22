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
    )

add_library(peer_server
    peer_server_libp2p.cpp
    peer_server_libp2p.hpp
    )
target_link_libraries(peer_server
    outcome
    buffer
    scale
    )
