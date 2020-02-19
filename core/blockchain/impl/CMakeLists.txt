#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(blockchain_common
    types.cpp
    common.hpp
    storage_util.cpp
    storage_util.hpp
    )
target_link_libraries(blockchain_common
    blob
    Boost::boost
    buffer
    database_error
    in_memory_storage
    polkadot_trie_db
    trie_db_backend
    primitives
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
