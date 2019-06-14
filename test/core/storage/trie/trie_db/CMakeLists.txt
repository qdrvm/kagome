# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(polkadot_trie_db_test
    polkadot_codec_nibbles_test.cpp
    polkadot_codec_hash256_test.cpp
    polkadot_codec_node_encoding_test.cpp
    polkadot_codec_node_decoding_test.cpp
    trie_db_test.cpp
    trie_batch_test.cpp
    ordered_trie_hash_test.cpp
    )
target_link_libraries(polkadot_trie_db_test
    leveldb
    polkadot_trie_db
    polkadot_trie_batch
    polkadot_trie_db_codec
    base_leveldb_test
    scale
    buffer
    in_memory_storage
    ordered_trie_hash
    )
