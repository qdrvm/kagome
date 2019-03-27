#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(libp2p-peer
    peer_id.hpp
    peer_id.cpp
    peer_id_manager.hpp
    peer_id_manager.cpp
    )
target_link_libraries(libp2p-peer
    buffer
    multihash
    sha
    outcome
    )
