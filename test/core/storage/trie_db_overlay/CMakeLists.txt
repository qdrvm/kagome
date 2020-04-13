# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(trie_db_overlay_test
    trie_db_overlay_test.cpp
    )
target_link_libraries(trie_db_overlay_test
    trie_db_overlay
    polkadot_trie_db
    )
