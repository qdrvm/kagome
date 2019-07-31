# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_protocol_kademlia
    kad_impl.cpp
    )
target_link_libraries(libp2p_protocol_kademlia
    libp2p_peer_id
    Boost::boost
    )
