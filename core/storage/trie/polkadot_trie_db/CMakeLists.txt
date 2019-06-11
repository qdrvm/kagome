# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(polkadot_trie_db_codec
    polkadot_codec.cpp
    polkadot_node.cpp
    )
target_link_libraries(polkadot_trie_db_codec
    buffer
    outcome
    blake2
    blob
    scale
    )

add_library(polkadot_trie_db
    polkadot_trie_db.cpp
    )
target_link_libraries(polkadot_trie_db
    polkadot_trie_db_codec
    polkadot_trie
    )

add_library(polkadot_trie
        polkadot_trie.cpp
        )
target_link_libraries(polkadot_trie
        buffer
        outcome
        )

add_library(polkadot_trie_batch
    polkadot_trie_batch.cpp
    )
target_link_libraries(polkadot_trie_batch
    polkadot_trie_db
    )

add_library(ordered_trie_hash INTERFACE)

target_link_libraries(ordered_trie_hash INTERFACE
    polkadot_trie
    polkadot_trie_db_codec
    scale
    )
