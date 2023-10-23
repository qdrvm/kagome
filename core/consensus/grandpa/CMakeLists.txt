#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(grandpa
    impl/authority_manager_impl.cpp
    impl/vote_tracker_impl.cpp
    impl/vote_crypto_provider_impl.cpp
    impl/grandpa_impl.cpp
    impl/voting_round_impl.cpp
    impl/environment_impl.cpp
    authority_manager_error.cpp
    vote_graph/vote_graph_error.cpp
    vote_graph/vote_graph_impl.cpp
    voter_set.cpp
    voting_round_error.cpp
    )
target_link_libraries(grandpa
#    mp_utils
#    ed25519_types
#    Boost::boost
#    schnorrkel_crust::schnorrkel_crust
#    transaction_pool_error
#    hasher
#    vrf_provider
    logger
#    scale::scale
#    primitives
#    blob
#    outcome
#    p2p::p2p_peer_id
#    storage
    metrics
#    blockchain
#    telemetry
    )
kagome_install(grandpa)
