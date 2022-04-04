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
    scale::scale
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
    scale::scale
    block_storage_error
    )

add_library(block_executor
    block_executor_impl.cpp
    )
target_link_libraries(block_executor
    logger
    primitives
    scale::scale
    block_tree_error
    threshold_util
    transaction_pool_error
    babe_digests_util
    metrics
    telemetry
    blockchain_common
    )

add_library(babe_util
    babe_util_impl.cpp
    )
target_link_libraries(babe_util
    primitives
    scale::scale
    babe_digests_util
    )

add_library(babe
    babe_impl.cpp
    babe_impl.hpp
    )
target_link_libraries(babe
    babe_error
    Boost::boost
    schnorrkel_crust::schnorrkel_crust
    buffer
    logger
    scale::scale
    blob
    outcome
    p2p::p2p_peer_id
    babe_digests_util
    threshold_util
    block_executor
    polkadot_trie
    metrics
    block_storage_error
    )
