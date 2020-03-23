# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(polkadot_trie_codec
    polkadot_codec.cpp
    )
target_link_libraries(polkadot_trie_codec
    buffer
    blake2
    blob
    scale
    )
kagome_install(polkadot_trie_codec)

add_library(polkadot_node
    polkadot_node.cpp
    )
target_link_libraries(polkadot_node
    buffer
    )
kagome_install(polkadot_node)

add_library(trie_db_backend
    trie_db_backend_impl.cpp
    polkadot_trie_db_backend_batch.cpp
    )
target_link_libraries(trie_db_backend
    buffer
    polkadot_node
    )
kagome_install(trie_db_backend)

add_library(polkadot_trie_db
    polkadot_trie_db.cpp
    )
target_link_libraries(polkadot_trie_db
    polkadot_trie_codec
    polkadot_trie
    )
kagome_install(polkadot_trie_db)

add_library(polkadot_trie
    polkadot_trie.cpp
    trie_error.cpp
    )
target_link_libraries(polkadot_trie
    buffer
    polkadot_trie_batch
    polkadot_node
    )
kagome_install(polkadot_trie)

add_library(polkadot_trie_cursor
    polkadot_trie_cursor.cpp
    )
target_link_libraries(polkadot_trie_cursor
    polkadot_trie
    )

add_library(polkadot_trie_batch
    polkadot_trie_batch.cpp
    )
target_link_libraries(polkadot_trie_batch
    polkadot_trie_db
    )
kagome_install(polkadot_trie_batch)

add_library(ordered_trie_hash INTERFACE)

target_link_libraries(ordered_trie_hash INTERFACE
    polkadot_trie
    polkadot_trie_codec
    scale
    )
kagome_install(ordered_trie_hash)
