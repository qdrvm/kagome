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
    impl/votes_cache.cpp
    impl/environment_impl.cpp
    authority_manager_error.cpp
    vote_graph/vote_graph_error.cpp
    vote_graph/vote_graph_impl.cpp
    voter_set.cpp
    voting_round_error.cpp
    )
target_link_libraries(grandpa
    logger
    metrics
    )
kagome_install(grandpa)
kagome_clear_objects(grandpa)
