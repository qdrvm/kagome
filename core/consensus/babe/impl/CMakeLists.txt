#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(babe_lottery
    babe_lottery_impl.cpp
    babe_lottery_impl.hpp
    )
target_link_libraries(babe_lottery
    hasher
    vrf_provider
    )

add_library(babe
    babe_impl.cpp
    babe_impl.hpp
    )
target_link_libraries(babe
    babe_error
    Boost::boost
    sr25519::sr25519
    buffer
    logger
    scale
    blob
    outcome
    libp2p_peer_id
    primitives
    )

add_library(babe_observer
    babe_observer_impl.cpp
    babe_observer_impl.hpp
    )
target_link_libraries(babe_observer
    babe
    )
