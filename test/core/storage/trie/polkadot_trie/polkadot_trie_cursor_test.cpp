/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"

#include <random>

#include <gtest/gtest.h>

#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/polkadot_trie_printer.hpp"

using kagome::common::Buffer;
using kagome::storage::trie::PolkadotTrie;
using kagome::storage::trie::PolkadotTrieCursor;
using kagome::storage::trie::PolkadotTrieImpl;
using kagome::storage::trie::operator<<;

class PolkadotTrieCursorTest : public testing::Test {};

// The default values for arguments are somewhat randomly chosen,
// they totally depend on what you want to test.
// More alphabet size ~ more branching
// Longer keys ~ longer keys (increases branching, too, if alphabet size is big
// enough, and can be used for performance testing
std::tuple<std::unique_ptr<PolkadotTrie>, std::set<Buffer>> generateRandomTrie(
    size_t keys_num,
    size_t max_key_length = 32,
    size_t key_alphabet_size = 16) noexcept {
  std::tuple<std::unique_ptr<PolkadotTrie>, std::set<Buffer>> res;
  auto trie = std::make_unique<PolkadotTrieImpl>();
  std::mt19937 eng(5489u);  // explicitly set default seed
  std::uniform_int_distribution<std::mt19937::result_type> key_dist(
      0, key_alphabet_size);
  std::uniform_int_distribution<std::mt19937::result_type> length_dist(
      1, max_key_length);
  auto key_gen = std::bind(key_dist, eng);
  auto key_length_gen = std::bind(length_dist, eng);

  auto &keys = std::get<1>(res);
  for (size_t i = 0; i < keys_num; i++) {
    kagome::common::Buffer key(key_length_gen(), 0);
    std::generate(key.begin(), key.end(), std::ref(key_gen));
    EXPECT_OUTCOME_TRUE_1(trie->put(key, key));
    keys.emplace(std::move(key));
  }
  std::get<0>(res) = std::move(trie);
  return res;
}

std::shared_ptr<PolkadotTrie> makeTrie(
    const std::vector<std::pair<Buffer, Buffer>> &vals) {
  auto trie = std::make_shared<PolkadotTrieImpl>(
      [](auto b, uint8_t idx) { return b->children.at(idx); });
  for (auto &p : vals) {
    EXPECT_OUTCOME_TRUE_1(trie->put(p.first, p.second));
  }
  return trie;
}

TEST_F(PolkadotTrieCursorTest, NextOnRootOnlyTrie) {
  auto trie = makeTrie({{"a"_buf, Buffer{1}}});
  PolkadotTrieCursor cursor{*trie};
  ASSERT_FALSE(cursor.isValid());
  EXPECT_OUTCOME_SUCCESS(r1, cursor.next());
  ASSERT_TRUE(cursor.isValid());
  EXPECT_OUTCOME_SUCCESS(r2, cursor.next());
  ASSERT_FALSE(cursor.isValid());
}

TEST_F(PolkadotTrieCursorTest, NextOnEmptyTrie) {
  auto trie = makeTrie({});
  PolkadotTrieCursor cursor{*trie};
  ASSERT_FALSE(cursor.isValid());
  EXPECT_OUTCOME_SOME_ERROR(r1, cursor.next());
  ASSERT_FALSE(cursor.isValid());
}

TEST_F(PolkadotTrieCursorTest, NextOnSmallTrie) {
  std::vector<std::pair<Buffer, Buffer>> vals{{"ab"_buf, Buffer{1}},
                                              {"ac"_buf, Buffer{3}},
                                              {"acd"_buf, Buffer{2}},
                                              {"e"_buf, Buffer{7}},
                                              {"f"_buf, Buffer{8}},
                                              {"fg"_buf, Buffer{4}},
                                              {"fh"_buf, Buffer{5}},
                                              {"fhi"_buf, Buffer{6}}};
  auto trie = makeTrie(vals);
  PolkadotTrieCursor cursor{*trie};
  for (auto &p : vals) {
    EXPECT_OUTCOME_SUCCESS(r1, cursor.next());
    EXPECT_OUTCOME_TRUE(v, cursor.value());
    EXPECT_OUTCOME_TRUE(k, cursor.key());
    ASSERT_EQ(v, p.second);
    ASSERT_EQ(k, p.first);
  }
  EXPECT_OUTCOME_SUCCESS(r1, cursor.next());
  ASSERT_FALSE(cursor.isValid());
}

/**
 * Stress-test for the trie cursor
 * @given a generated big tree (using a pseudo-random generator to avoid a
 * floating test)
 * @when traversing using a cursor it from every key
 * @then every key lexicographically bigger than current is visited once
 */
TEST_F(PolkadotTrieCursorTest, BigPseudoRandomTrieRandomStart) {
  auto &&[trie, keys] = generateRandomTrie(100, 8, 32);
  const auto cursor = trie->cursor();
  EXPECT_OUTCOME_TRUE_1(cursor->next());
  size_t keys_behind =
      0;  // number of keys lexicographically lesser than current
  for (auto start_key : keys) {
    EXPECT_OUTCOME_TRUE_1(cursor->seek(start_key));
    auto keys_copy = std::set<Buffer>(keys);
    while (cursor->isValid()) {
      EXPECT_OUTCOME_TRUE(key, cursor->key());
      EXPECT_OUTCOME_TRUE(value, cursor->value());
      ASSERT_EQ(key, value);
      ASSERT_TRUE(keys_copy.find(key) != keys_copy.end());
      keys_copy.erase(key);
      EXPECT_OUTCOME_TRUE_1(cursor->next());
    }
    ASSERT_EQ(keys_copy.size(),
              keys_behind++);  // unvisited keys are those already visited when
                               // starting from lesser keys
  }
}

/**
 * @given a trie
 * @when traversing it with a cursor
 * @then it visits keys in lexicographical order
 */
TEST_F(PolkadotTrieCursorTest, Lexicographical) {
  std::vector<std::pair<Buffer, Buffer>> vals{
      {"0102"_hex2buf, "0102"_hex2buf},
      {"0103"_hex2buf, "0103"_hex2buf},
      {"010304"_hex2buf, "010304"_hex2buf},
      {"05"_hex2buf, "05"_hex2buf},
      {"06"_hex2buf, "06"_hex2buf},
      {"0607"_hex2buf, "0607"_hex2buf},
      {"060708"_hex2buf, "060708"_hex2buf},
      {"06070801"_hex2buf, "06070801"_hex2buf},
      {"06070802"_hex2buf, "06070802"_hex2buf},
      {"06070803"_hex2buf, "06070803"_hex2buf}};
  auto trie = makeTrie(vals);
  auto c = trie->cursor();
  EXPECT_OUTCOME_FALSE_1(c->seek("f"_buf));
  EXPECT_OUTCOME_TRUE_1(c->seek("06"_hex2buf));
  Buffer prev_key{0};
  do {
    EXPECT_OUTCOME_TRUE(key, c->key());
    ASSERT_LT(prev_key, key);
    prev_key = key;
    EXPECT_OUTCOME_TRUE_1(c->next());
  } while (c->isValid());
}
