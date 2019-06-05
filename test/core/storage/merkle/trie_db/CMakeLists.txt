# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(polkadot_trie_db_test
    polkadot_codec_nibbles_test.cpp
    polkadot_codec_hash256_test.cpp
    polkadot_codec_node_encoding_test.cpp
    )
target_link_libraries(polkadot_trie_db_test
    polkadot_trie_db_codec
    scale
    buffer
    )
