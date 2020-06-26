# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(polkadot_node
    polkadot_node.cpp
    )
target_link_libraries(polkadot_node
    buffer
    )
kagome_install(polkadot_node)

add_library(trie_error
    trie_error.cpp
    )
target_link_libraries(trie_error
    outcome
    )
kagome_install(trie_error)

add_library(polkadot_trie_cursor
    polkadot_trie_cursor.cpp
    )
target_link_libraries(polkadot_trie_cursor
    buffer
    polkadot_codec
    )
kagome_install(polkadot_trie_cursor)

add_library(polkadot_trie
    polkadot_trie_impl.cpp
    )
target_link_libraries(polkadot_trie
    polkadot_node
    trie_error
    polkadot_codec
    polkadot_trie_cursor
    )
kagome_install(polkadot_trie)

add_library(polkadot_trie_factory
    polkadot_trie_factory_impl.cpp
    )
target_link_libraries(polkadot_trie_factory
    polkadot_trie
    )
