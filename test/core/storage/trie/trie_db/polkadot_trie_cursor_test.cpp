/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/polkadot_trie_cursor.hpp"

#include <gtest/gtest.h>

namespace trie = kagome::storage::trie;

TEST(PolkadotTrieCursorTest, TODO) {

  auto trie = std::make_shared<trie::PolkadotTrie>([](auto b, uint8_t idx) { return b->children[idx]; });
  trie::PolkadotTrieCursor cursor {trie};
}
