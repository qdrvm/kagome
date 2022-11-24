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
kagome_install(threshold_util)

add_library(babe_digests_util
    babe_digests_util.cpp
    )
target_link_libraries(babe_digests_util
    primitives
    scale::scale
    blockchain
    )
kagome_install(babe_digests_util)

add_library(block_appender
    block_appender_impl.cpp
    )
target_link_libraries(block_appender
    logger
    primitives
    scale::scale
    blockchain
    threshold_util
    babe_digests_util
    )

add_library(block_executor
    block_executor_impl.cpp
    )
target_link_libraries(block_executor
    logger
    primitives
    scale::scale
    blockchain
    threshold_util
    transaction_pool_error
    babe_digests_util
    metrics
    telemetry
    consistency_keeper
    )
kagome_install(block_executor)

add_library(consistency_keeper
    consistency_keeper_impl.cpp
    )
target_link_libraries(consistency_keeper
    Boost::boost
    blockchain
    )
kagome_install(consistency_keeper)

add_library(babe_config_repository
    babe_config_repository_impl.cpp
    babe_config_node.cpp
    )
target_link_libraries(babe_config_repository
    primitives
    scale::scale
    )

add_library(babe
    babe_impl.cpp
    babe_impl.hpp
    )
target_link_libraries(babe
    babe_error
    Boost::boost
    schnorrkel_crust::schnorrkel_crust
    logger
    scale::scale
    blob
    outcome
    p2p::p2p_peer_id
    babe_digests_util
    threshold_util
    block_executor
    storage
    metrics
    blockchain
    telemetry
    )
kagome_install(babe)
