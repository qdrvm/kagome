# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(polkadot_node
    polkadot_node.cpp
    )
target_link_libraries(polkadot_node
    buffer
    )

add_library(polkadot_trie
    polkadot_trie_impl.cpp
    )
target_link_libraries(polkadot_trie
    polkadot_node
    )

add_library(polkadot_trie_factory
    polkadot_trie_factory_impl.cpp
    )
target_link_libraries(polkadot_trie_factory
    polkadot_trie
    )

add_library(trie_serializer
    trie_serializer_impl.cpp
    )
target_link_libraries(trie_serializer
    polkadot_node
    )

add_library(polkadot_codec
    polkadot_codec.cpp
    )
target_link_libraries(polkadot_codec
    scale
    )

add_library(trie_storage_backend
    trie_storage_backend_impl.cpp
    trie_storage_backend_batch.cpp
    )
target_link_libraries(trie_storage_backend
    buffer
    )

add_library(persistent_trie_batch
    persistent_trie_batch_impl.cpp
    )
target_link_libraries(persistent_trie_batch
    buffer
    )

add_library(ephemeral_trie_batch
    ephemeral_trie_batch_impl.cpp
    )
target_link_libraries(ephemeral_trie_batch
    buffer
    )

add_library(trie_storage
    trie_storage_impl.cpp
    )
target_link_libraries(trie_storage
    ephemeral_trie_batch
    persistent_trie_batch
    )
