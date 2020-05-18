# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(changes_tracker
    impl/storage_changes_tracker_impl.cpp
    impl/changes_trie.cpp
    )
target_link_libraries(changes_tracker
    buffer
    blob
    )

add_library(changes_trie_builder
    impl/changes_trie_builder_impl.cpp
    )
target_link_libraries(changes_trie_builder
    buffer
    blob
    logger
    scale
    )
