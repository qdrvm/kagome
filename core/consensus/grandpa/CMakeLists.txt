#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(vote_graph)

add_library(environment
    impl/environment_impl.cpp
    )

target_link_libraries(environment
    scale::scale
    logger
    blob
    )

add_library(vote_tracker
    impl/vote_tracker_impl.cpp
    )
target_link_libraries(vote_tracker
    blob
    )

add_library(vote_crypto_provider
    impl/vote_crypto_provider_impl.cpp
    )
target_link_libraries(vote_crypto_provider
    scale::scale
    outcome
    )

add_library(voter_set
    voter_set.cpp
    )
target_link_libraries(voter_set
    ed25519_types
    )

add_library(voting_round_error
    impl/voting_round_error.cpp
    )
target_link_libraries(voting_round_error
    outcome
    )

add_library(voting_round
    impl/voting_round_impl.cpp
    )

target_link_libraries(voting_round
    logger
    voter_set
    voting_round_error
    blockchain_common
    block_tree_error
    )

add_library(grandpa
    impl/grandpa_impl.cpp
    )
target_link_libraries(grandpa
    voting_round
    vote_crypto_provider
    vote_graph
    vote_tracker
    metrics
    )
