# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(polkadot_node
    trie_node.cpp
    )
target_link_libraries(polkadot_node
    fmt::fmt
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
    polkadot_trie_cursor_impl.cpp
    )
target_link_libraries(polkadot_trie_cursor
    polkadot_codec
    logger
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
kagome_install(polkadot_trie_factory)
