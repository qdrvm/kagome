#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(libp2p_peer_identity
    peer_identity.cpp
    peer_identity.hpp
    peer_identity_factory_impl.cpp
    peer_identity_factory_impl.hpp
    peer_identity_factory.hpp
    )
target_link_libraries(libp2p_peer_identity
    outcome
    multiaddress
    multihash
    )
