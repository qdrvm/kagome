#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(blockchain_common
    common.cpp
    common.hpp
    level_db_util.cpp
    level_db_util.hpp
    )
target_link_libraries(blockchain_common
    Boost::boost
    buffer
    primitives
    blob
    )

add_library(level_db_block_tree
    block_tree_impl.cpp
    block_tree_impl.hpp
    )
target_link_libraries(level_db_block_tree
    block_tree_error
    blockchain_common
    leveldb
    hasher
    )
