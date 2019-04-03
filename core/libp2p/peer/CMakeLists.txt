#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(peer
    peer_info.hpp
    peer_info.cpp
    peer_id.hpp
    peer_id.cpp
    )
target_link_libraries(peer
    multiaddress
    multihash
    outcome
    )
