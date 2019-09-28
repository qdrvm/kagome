#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#
addtest(vote_graph_test
    adjust_base.cpp
    walk_back_at_node.cpp
    walk_back_from_fork_block_node_below.cpp
    walk_back_from_block_in_edge_fork_below.cpp
    ghost_introduce_branch.cpp
    ghost_merge_not_at_node_one_side_weighted.cpp
    ghost_merge_at_node.cpp
    graph_fork.cpp
    )
target_link_libraries(vote_graph_test
    vote_graph
    blob
    )
