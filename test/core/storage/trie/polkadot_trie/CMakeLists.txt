#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#


addtest(polkadot_trie_test
    polkadot_trie_test.cpp
    )
target_link_libraries(polkadot_trie_test
    storage
    Boost::boost
    log_configurator
    )

addtest(polkadot_trie_cursor_test
    polkadot_trie_cursor_test.cpp
    )
target_link_libraries(polkadot_trie_cursor_test
    storage
    log_configurator
    )
