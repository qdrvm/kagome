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
    logger
    )

add_library(threshold_util
    threshold_util.cpp
    )
target_link_libraries(threshold_util
    primitives
    )

add_library(babe_digests_util
    babe_digests_util.cpp
    )
target_link_libraries(babe_digests_util
    primitives
    scale
    )

add_library(block_executor
    block_executor.cpp
    )
target_link_libraries(block_executor
    logger
    primitives
    scale
    block_tree_error
    threshold_util
    transaction_pool_error
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
    p2p::p2p_peer_id
    babe_digests_util
    threshold_util
    block_executor
    )

add_library(babe_synchronizer
    babe_synchronizer_impl.cpp
    )
target_link_libraries(babe_synchronizer
    logger
    primitives
    )

add_library(syncing_babe_observer
    syncing_babe_observer.cpp
    )
target_link_libraries(syncing_babe_observer
    logger
    primitives
    )
