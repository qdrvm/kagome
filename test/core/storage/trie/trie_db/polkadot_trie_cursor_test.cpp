/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/polkadot_trie_cursor.hpp"

#include <gtest/gtest.h>

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

namespace trie = kagome::storage::trie;

using kagome::common::Buffer;

std::shared_ptr<trie::PolkadotTrie> makeTrie(const std::vector<std::pair<Buffer, Buffer>>& vals) {
  auto trie = std::make_shared<trie::PolkadotTrie>([](auto b, uint8_t idx) { return b->getChild(idx); });
  for (auto& p: vals) {
    EXPECT_OUTCOME_TRUE_1(trie->put(p.first, p.second));
  }
  return trie;
}

TEST(PolkadotTrieCursorTest, NextOnRootOnlyTrie) {
  trie::PolkadotTrieCursor cursor {makeTrie({{"a"_buf, Buffer{1}}})};
  ASSERT_TRUE(cursor.isValid());
  cursor.next();
  ASSERT_FALSE(cursor.isValid());
}

TEST(PolkadotTrieCursorTest, NextOnEmptyTrie) {
  trie::PolkadotTrieCursor cursor {makeTrie({})};
  ASSERT_FALSE(cursor.isValid());
  cursor.next();
  ASSERT_FALSE(cursor.isValid());
}

TEST(PolkadotTrieCursorTest, NextOnSmallTrie) {
  std::vector<std::pair<Buffer, Buffer>> vals {
      {"ab"_buf, Buffer{1}},
      {"ac"_buf, Buffer{3}},
      {"acd"_buf, Buffer{2}},
      {"e"_buf, Buffer{7}},
      {"f"_buf, Buffer{8}},
      {"fg"_buf, Buffer{4}},
      {"fh"_buf, Buffer{5}},
      {"fhi"_buf, Buffer{6}}
  };

  trie::PolkadotTrieCursor cursor {makeTrie(vals)};
  for (auto& p: vals) {
    cursor.next();
    EXPECT_OUTCOME_TRUE(v, cursor.value());
    EXPECT_OUTCOME_TRUE(k, cursor.key());
    ASSERT_EQ(v, p.second);
    //ASSERT_EQ(k, p.first);
  }
  cursor.next();
  ASSERT_FALSE(cursor.isValid());
}
