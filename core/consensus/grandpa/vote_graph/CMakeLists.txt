#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(vote_graph
    vote_graph_impl.cpp
    )
target_link_libraries(vote_graph
    Boost::boost
    outcome
    )
