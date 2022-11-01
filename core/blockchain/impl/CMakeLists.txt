#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(blockchain_common
    common.cpp
    storage_util.cpp
    )
target_link_libraries(blockchain_common
    blob
    Boost::boost
    database_error
    in_memory_storage
    trie_storage
    trie_storage_backend
    polkadot_trie
    polkadot_codec
    primitives
    )
kagome_install(blockchain_common)

add_library(justification_storage_policy
    justification_storage_policy.cpp
    )
target_link_libraries(justification_storage_policy
    logger
    blob
)
kagome_install(justification_storage_policy)

add_library(block_tree
    block_tree_impl.cpp
    cached_tree.cpp
    )
target_link_libraries(block_tree
    block_tree_error
    block_storage_error
    blockchain_common
    hasher
    metrics
    justification_storage_policy
    telemetry
    )
