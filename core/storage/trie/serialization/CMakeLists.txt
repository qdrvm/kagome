# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

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
    blake2
    blob
    polkadot_node
    )
kagome_install(polkadot_codec)

add_library(ordered_trie_hash INTERFACE)
target_link_libraries(ordered_trie_hash INTERFACE
    polkadot_trie
    polkadot_codec
    scale
    )
kagome_install(ordered_trie_hash)
