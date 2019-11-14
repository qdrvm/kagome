#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(vote_graph)

add_library(chain
    impl/chain_impl.cpp
    )

target_link_libraries(chain
    block_tree
    block_header_repository
    )

add_library(vote_tracker
    impl/vote_tracker_impl.cpp)

target_link_libraries(vote_tracker
    blob
    )

add_library(voting_round
    impl/voting_round_impl.cpp
    )

target_link_libraries(voting_round
    vote_tracker
    vote_graph
    scale
    logger
    blob
    )

add_library(launcher
    impl/launcher_impl.cpp
    )
target_link_libraries(launcher
    voting_round
    )
