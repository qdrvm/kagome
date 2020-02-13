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

add_library(observer_dummy
    impl/observer_dummy.cpp
    )
target_link_libraries(observer_dummy
    logger
    )
