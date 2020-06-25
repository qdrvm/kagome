# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0


addtest(polkadot_trie_test
    polkadot_trie_test.cpp
    )
target_link_libraries(polkadot_trie_test
    polkadot_trie_factory
    Boost::boost
    trie_serializer
    trie_error
    )

addtest(polkadot_trie_cursor_test
    polkadot_trie_cursor_test.cpp
    )
target_link_libraries(polkadot_trie_cursor_test
    polkadot_trie_cursor
    polkadot_trie
    )
