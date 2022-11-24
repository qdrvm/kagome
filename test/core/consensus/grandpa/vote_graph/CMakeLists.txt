#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#
addtest(vote_graph_test
    adjust_base_test.cpp
    walk_back_at_node_test.cpp
    walk_back_from_fork_block_node_below_tests.cpp
    walk_back_from_block_in_edge_fork_below_test.cpp
    ghost_introduce_branch_test.cpp
    ghost_merge_not_at_node_one_side_weighted_test.cpp
    ghost_merge_at_node_test.cpp
    graph_fork_test.cpp
    duplicate_vote_test.cpp
    remove_vote_test.cpp
    small_block_number_test.cpp
    )
target_link_libraries(vote_graph_test
    vote_graph
    vote_tracker
    voter_set
    blob
    blockchain
    )
