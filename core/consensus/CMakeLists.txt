#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(consensus
    authority/impl/schedule_node.cpp
    authority/impl/authority_manager_impl.cpp
    authority/authority_update_observer_error.cpp
    authority/authority_manager_error.cpp
    babe/impl/babe_digests_util.cpp
    babe/impl/babe_config_node.cpp
    babe/impl/block_executor_impl.cpp
    babe/impl/babe_impl.cpp
    babe/impl/threshold_util.cpp
    babe/impl/babe_lottery_impl.cpp
    babe/impl/babe_config_repository_impl.cpp
    babe/impl/block_appender_impl.cpp
    babe/impl/consistency_keeper_impl.cpp
    babe/babe_error.cpp
    validation/babe_block_validator.cpp
    grandpa/impl/vote_tracker_impl.cpp
    grandpa/impl/vote_crypto_provider_impl.cpp
    grandpa/impl/grandpa_impl.cpp
    grandpa/impl/voting_round_impl.cpp
    grandpa/impl/environment_impl.cpp
    grandpa/vote_graph/vote_graph_error.cpp
    grandpa/vote_graph/vote_graph_impl.cpp
    grandpa/voter_set.cpp
    grandpa/voting_round_error.cpp
    )
target_link_libraries(consensus
    mp_utils
    ed25519_types
    Boost::boost
    schnorrkel_crust::schnorrkel_crust
    transaction_pool_error
    hasher
    vrf_provider
    logger
    scale::scale
    primitives
    blob
    outcome
    p2p::p2p_peer_id
    storage
    metrics
    blockchain
    telemetry
    )
kagome_install(consensus)
add_custom_command(TARGET consensus
    POST_BUILD
    COMMAND find ${CMAKE_CURRENT_BINARY_DIR} -name "*.o" -type f -delete
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Removing object files from '${CMAKE_CURRENT_BINARY_DIR}'"
)
