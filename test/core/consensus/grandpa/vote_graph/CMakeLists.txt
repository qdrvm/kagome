#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
#
# Kagome is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Kagome is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    )
target_link_libraries(vote_graph_test
    vote_graph
    vote_tracker
    blob
    block_tree
    )
