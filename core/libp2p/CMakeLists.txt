#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(multi)
add_subdirectory(transport)
add_subdirectory(crypto)
add_subdirectory(peer)
add_subdirectory(network)
add_subdirectory(protocol_muxer)
add_subdirectory(muxer)
add_subdirectory(security)
add_subdirectory(connection)
add_subdirectory(protocol)
add_subdirectory(basic)
add_subdirectory(host)

add_library(libp2p
    libp2p.cpp
    )
target_link_libraries(libp2p INTERFACE
    libp2p_network
    libp2p_tcp
    libp2p_yamux
    libp2p_plaintext
    libp2p_connection_manager
    libp2p_transport_manager
    libp2p_listener_manager
    libp2p_identity_manager
    libp2p_dialer
    libp2p_router
    multiselect
    random_generator
    key_generator
    marshaller
    )
