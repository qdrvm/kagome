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
# TODO(warchant): PRE-176 refactor yamux
#add_subdirectory(muxer)
add_subdirectory(security)
add_subdirectory(connection)

add_library(libp2p
    config.hpp
    host.cpp
    host.hpp
    host_builder.cpp
    host_builder.hpp
    )
target_link_libraries(libp2p
    outcome
    random_generator
    libp2p_peer_id
    multiaddress
    libp2p_peer_repository
    address_repository
    key_generator
    buffer
    libp2p_inmem_address_repository
    libp2p_inmem_key_repository
    libp2p_inmem_protocol_repository
    security_plaintext
    )
