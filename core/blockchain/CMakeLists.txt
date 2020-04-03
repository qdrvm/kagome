#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(changes_trie_builder
    impl/changes_trie_builder_impl.cpp
    )
target_link_libraries(changes_trie_builder
    buffer
    )

add_library(block_header_repository
    impl/key_value_block_header_repository.cpp
    impl/block_header_repository.cpp
    )
target_link_libraries(block_header_repository
    blockchain_common
    )

add_library(block_storage
    impl/key_value_block_storage.cpp
    )
target_link_libraries(block_storage
    blockchain_common
    hasher
    scale
    leveldb
    )

add_library(block_tree_error
    block_tree_error.hpp
    impl/block_tree_error.cpp
    )
target_link_libraries(block_tree_error
    outcome
    )
