#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(blockchain_common
    common.cpp
    common.hpp
    persistant_map_util.cpp
    persistant_map_util.hpp
    )
target_link_libraries(blockchain_common
    Boost::boost
    buffer
    primitives
    blob
    )

add_library(block_tree
    block_tree_impl.cpp
    block_tree_impl.hpp
    )
target_link_libraries(block_tree
    block_tree_error
    blockchain_common
    leveldb
    hasher
    )
