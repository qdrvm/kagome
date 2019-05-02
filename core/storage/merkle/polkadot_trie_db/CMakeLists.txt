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
    )
