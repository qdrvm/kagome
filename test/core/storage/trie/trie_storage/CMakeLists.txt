#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(polkadot_trie_storage_test
    polkadot_codec_nibbles_test.cpp
    polkadot_codec_hash256_test.cpp
    polkadot_codec_node_encoding_test.cpp
    polkadot_codec_node_decoding_test.cpp
    trie_storage_test.cpp
    trie_batch_test.cpp
    ordered_trie_hash_test.cpp
    )
target_link_libraries(polkadot_trie_storage_test
    storage
    Boost::boost
    base_rocksdb_test
    logger_for_tests
    )

addtest(trie_storage_backend_test
    trie_storage_backend_test.cpp
    )
target_link_libraries(trie_storage_backend_test
    storage
    blob
    )
