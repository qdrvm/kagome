/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include "storage/trie/polkadot_trie/polkadot_trie_cursor_impl.hpp"

#include <random>

#include <gtest/gtest.h>

#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/storage/polkadot_trie_printer.hpp"

using kagome::common::Buffer;
using kagome::storage::trie::PolkadotTrie;
using kagome::storage::trie::PolkadotTrieCursorImpl;
using kagome::storage::trie::PolkadotTrieImpl;
using kagome::storage::trie::operator<<;

class PolkadotTrieCursorTest : public testing::Test {
 public:
  void SetUp() override {
    testutil::prepareLoggers();
  }

 private:
};

// The default values for arguments are somewhat randomly chosen,
// they totally depend on what you want to test.
// Bigger alphabet size ~ more branching
// Longer keys ~ longer keys (increases branching, too, if alphabet size is big
// enough, and can be used for performance testing
std::tuple<std::shared_ptr<PolkadotTrie>, std::set<Buffer>> generateRandomTrie(
    size_t keys_num,
    size_t max_key_length = 32,
    size_t key_alphabet_size = 16) noexcept {
  std::tuple<std::shared_ptr<PolkadotTrie>, std::set<Buffer>> res;
  auto trie = std::make_shared<PolkadotTrieImpl>();
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
    EXPECT_OUTCOME_TRUE_1(trie->put(key, key))
    keys.emplace(std::move(key));
  }
  std::get<0>(res) = std::move(trie);
  return res;
}

std::shared_ptr<PolkadotTrie> makeTrie(
    const std::vector<std::pair<Buffer, Buffer>> &vals) {
  auto trie = std::make_shared<PolkadotTrieImpl>();
  for (auto &p : vals) {
    EXPECT_OUTCOME_TRUE_1(trie->put(p.first, p.second))
  }
  return trie;
}

TEST_F(PolkadotTrieCursorTest, NextOnRootOnlyTrie) {
  auto trie = makeTrie({{"a"_buf, Buffer{1}}});
  PolkadotTrieCursorImpl cursor{trie};
  ASSERT_FALSE(cursor.isValid());
  EXPECT_OUTCOME_SUCCESS(r1, cursor.next())
  ASSERT_TRUE(cursor.isValid());
  EXPECT_OUTCOME_SUCCESS(r2, cursor.next())
  ASSERT_FALSE(cursor.isValid());
}

TEST_F(PolkadotTrieCursorTest, NextOnEmptyTrie) {
  auto trie = makeTrie({});
  PolkadotTrieCursorImpl cursor{trie};
  ASSERT_FALSE(cursor.isValid());
  EXPECT_OUTCOME_TRUE_1(cursor.next())
  ASSERT_FALSE(cursor.isValid());
}

TEST_F(PolkadotTrieCursorTest, NextOnSmallTrie) {
  std::vector<std::pair<Buffer, Buffer>> vals{{"ab"_buf, Buffer{1}},
                                              {"ac"_buf, Buffer{2}},
                                              {"acd"_buf, Buffer{3}},
                                              {"e"_buf, Buffer{7}},
                                              {"f"_buf, Buffer{8}},
                                              {"fg"_buf, Buffer{4}},
                                              {"fh"_buf, Buffer{5}},
                                              {"fhi"_buf, Buffer{6}}};
  auto trie = makeTrie(vals);
  std::cout << *trie << "\n";
  PolkadotTrieCursorImpl cursor{trie};
  for (auto &p : vals) {
    SCOPED_TRACE("Current key is " + p.first.toString());
    std::cout << "At " << p.first.toHex() << " " << p.first.toString() << "\n";
    EXPECT_OUTCOME_SUCCESS(r1, cursor.next())
    ASSERT_TRUE(cursor.key());
    ASSERT_TRUE(cursor.value());
    ASSERT_EQ(cursor.key().value(), p.first);
    ASSERT_EQ(cursor.value().value().get(), p.second);
  }
  EXPECT_OUTCOME_SUCCESS(r1, cursor.next())
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
  auto [trie, keys] = generateRandomTrie(100, 8, 32);
  const auto cursor = trie->cursor();
  EXPECT_OUTCOME_TRUE_1(cursor->next())
  size_t keys_behind =
      0;  // number of keys lexicographically lesser than current
  for (const auto &start_key : keys) {
    EXPECT_OUTCOME_TRUE_1(cursor->seek(start_key))
    auto keys_copy = std::set<Buffer>(keys);
    while (cursor->isValid()) {
      ASSERT_TRUE(cursor->key());
      ASSERT_TRUE(cursor->value());
      auto k = cursor->key().value();
      auto v = cursor->value().value();
      ASSERT_EQ(k, v);
      ASSERT_TRUE(keys_copy.find(k) != keys_copy.end());
      keys_copy.erase(k);
      EXPECT_OUTCOME_TRUE_1(cursor->next())
    }
    ASSERT_EQ(keys_copy.size(),
              keys_behind++);  // unvisited keys are those already visited when
                               // starting from lesser keys
  }
}

const std::vector<std::pair<Buffer, Buffer>> lex_sorted_vals{
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

/**
 * @given a trie
 * @when traversing it with a cursor
 * @then it visits keys in lexicographical order
 */
TEST_F(PolkadotTrieCursorTest, Lexicographical) {
  auto trie = makeTrie(lex_sorted_vals);
  auto cursor = trie->cursor();
  EXPECT_OUTCOME_TRUE(res, cursor->seek("f"_buf))
  ASSERT_FALSE(res);
  EXPECT_OUTCOME_TRUE_1(cursor->seek("06"_hex2buf))
  Buffer prev_key{0};
  do {
    ASSERT_TRUE(cursor->key());
    auto key = cursor->key().value();
    ASSERT_LT(prev_key, key);
    prev_key = key;
    EXPECT_OUTCOME_TRUE_1(cursor->next())
  } while (cursor->isValid());
}

/**
 * @given a non-empty trie
 * @when seeking a lower bound for a given byte sequence which is somewhere in
 * the middle in the set of trie keys and is not present in the trie
 * @then the corresponding lower bound is found
 */
TEST_F(PolkadotTrieCursorTest, LowerBoundKeyNotPresent) {
  auto trie = makeTrie(lex_sorted_vals);
  auto cursor = trie->trieCursor();
  cursor->seekLowerBound("06066666"_hex2buf).value();
  ASSERT_EQ(cursor->value().value().get(), "0607"_hex2buf);
  EXPECT_OUTCOME_TRUE_1(cursor->next())
  ASSERT_EQ(cursor->value().value().get(), "060708"_hex2buf);
}

/**
 * @given a non-empty trie
 * @when seeking a lower bound for a given byte sequence which is greater than
 * any key in the trie
 * @then the corresponding lower bound is found
 */
TEST_F(PolkadotTrieCursorTest, LowerBoundKeyGreatest) {
  auto trie = makeTrie(lex_sorted_vals);
  auto cursor = trie->trieCursor();
  cursor->seekLowerBound("060709"_hex2buf).value();
  ASSERT_FALSE(cursor->isValid());
}

/**
 * @given a non-empty trie
 * @when seeking a lower bound for a given byte sequence which is somewhere in
 * the middle in the set of trie keys
 * @then the corresponding lower bound is found
 */
TEST_F(PolkadotTrieCursorTest, LowerBoundMiddleFromRoot) {
  auto trie = makeTrie(lex_sorted_vals);
  auto cursor = trie->trieCursor();
  cursor->seekLowerBound("03"_hex2buf).value();
  ASSERT_EQ(cursor->value().value().get(), "05"_hex2buf);
  EXPECT_OUTCOME_TRUE_1(cursor->next())
  ASSERT_EQ(cursor->value().value().get(), "06"_hex2buf);
}

/**
 * @given a non-empty trie
 * @when seeking a lower bound for a byte sequence lex. smallest than any key in
 * the trie
 * @then the first key is found
 */
TEST_F(PolkadotTrieCursorTest, LowerBoundFirstKey) {
  auto trie = makeTrie(lex_sorted_vals);
  auto cursor = trie->trieCursor();

  cursor->seekLowerBound("00"_hex2buf).value();
  ASSERT_EQ(cursor->value().value().get(), "0102"_hex2buf);
  EXPECT_OUTCOME_TRUE_1(cursor->next())
  ASSERT_EQ(cursor->value().value().get(), "0103"_hex2buf);
}

/**
 * @given an empty trie
 * @when seeking a lower bound for a byte sequence in it
 * @then resulting iterator is not valid
 */
TEST_F(PolkadotTrieCursorTest, LowerBoundEmptyTrie) {
  auto trie = makeTrie({});
  auto cursor = trie->trieCursor();

  EXPECT_OUTCOME_TRUE_1(cursor->seekLowerBound("00"_hex2buf))
  ASSERT_FALSE(cursor->key().has_value());
}

/**
 * @given a non-empty trie
 * @when seeking a lower bound in the trie for a given byte sequence
 * @then the returned key is none if there is no lex. greater key in the trie
 */
TEST_F(PolkadotTrieCursorTest, LexOrderKept) {
  auto trie =
      makeTrie({{":heappages"_buf, "00"_hex2buf}, {":code"_buf, "geass"_buf}});
  auto cursor = trie->trieCursor();

  EXPECT_OUTCOME_TRUE_1(cursor->seekLowerBound("Optional"_buf))
  EXPECT_FALSE(cursor->key().has_value());
}

TEST_F(PolkadotTrieCursorTest, SeekFirst) {
  auto trie = makeTrie(lex_sorted_vals);
  auto cursor = trie->trieCursor();

  EXPECT_OUTCOME_TRUE_1(cursor->seekFirst())
  ASSERT_EQ(cursor->key().value(), lex_sorted_vals.front().first);
}

TEST_F(PolkadotTrieCursorTest, SeekLast) {
  auto trie = makeTrie(lex_sorted_vals);
  auto cursor = trie->trieCursor();

  EXPECT_OUTCOME_TRUE_1(cursor->seekLast())
  ASSERT_EQ(cursor->key().value(), lex_sorted_vals.back().first);
}

TEST_F(PolkadotTrieCursorTest, SeekWithNullRoot) {
  auto trie = makeTrie({});
  auto cursor = trie->trieCursor();

  EXPECT_OUTCOME_TRUE_1(cursor->seek("some_key"_buf))
  ASSERT_EQ(cursor->key(), std::nullopt);
  ASSERT_EQ(cursor->value(), std::nullopt);
}

TEST_F(PolkadotTrieCursorTest, SeekLastWithNullRoot) {
  auto trie = makeTrie({});
  auto cursor = trie->trieCursor();

  EXPECT_OUTCOME_TRUE_1(cursor->seekLast())
  ASSERT_EQ(cursor->key(), std::nullopt);
  ASSERT_EQ(cursor->value(), std::nullopt);
}

TEST_F(PolkadotTrieCursorTest, SeekUpperBound) {
  auto trie = makeTrie(lex_sorted_vals);
  auto cursor = trie->trieCursor();

  EXPECT_OUTCOME_TRUE_1(cursor->seekUpperBound(lex_sorted_vals[4].first))
  ASSERT_EQ(cursor->key().value(), lex_sorted_vals[5].first);

  EXPECT_OUTCOME_TRUE_1(cursor->seekUpperBound(lex_sorted_vals.back().first))
  ASSERT_EQ(cursor->key(), std::nullopt);

  EXPECT_OUTCOME_TRUE_1(cursor->seekUpperBound(lex_sorted_vals.front().first))
  ASSERT_EQ(cursor->key().value(), lex_sorted_vals[1].first);
}

TEST_F(PolkadotTrieCursorTest, SuccessfulCreateAt) {
  auto trie = makeTrie(lex_sorted_vals);
  EXPECT_OUTCOME_TRUE(cursor,
                      kagome::storage::trie::PolkadotTrieCursorImpl::createAt(
                          lex_sorted_vals[4].first, trie))
  ASSERT_EQ(cursor->key(), lex_sorted_vals[4].first);
}

TEST_F(PolkadotTrieCursorTest, CreateAtNonexisting) {
  auto trie = makeTrie(lex_sorted_vals);
  EXPECT_OUTCOME_FALSE_1(
      kagome::storage::trie::PolkadotTrieCursorImpl::createAt(
          "some_random_key"_buf, trie))
}

TEST_F(PolkadotTrieCursorTest, SeekNonexisting) {
  auto trie = makeTrie(lex_sorted_vals);
  auto cursor = trie->trieCursor();

  EXPECT_OUTCOME_TRUE_1(cursor->seek("some_random_key"_buf))
  ASSERT_FALSE(cursor->isValid());
}

TEST_F(PolkadotTrieCursorTest, SeekBranchNoValue) {
  auto trie = makeTrie(lex_sorted_vals);
  auto cursor = trie->trieCursor();

  EXPECT_OUTCOME_TRUE_1(cursor->seek("01"_hex2buf))
  ASSERT_FALSE(cursor->isValid());
}

TEST_F(PolkadotTrieCursorTest, SeekFirstEmptyTrie) {
  auto trie = makeTrie({});
  auto cursor = trie->trieCursor();

  EXPECT_OUTCOME_TRUE_1(cursor->seekFirst())
  ASSERT_FALSE(cursor->isValid());
}

TEST_F(PolkadotTrieCursorTest, SeekLowerBoundLeaf) {
  auto trie = makeTrie(lex_sorted_vals);
  auto cursor = trie->trieCursor();

  EXPECT_OUTCOME_TRUE_1(cursor->seekLowerBound(lex_sorted_vals[3].first))
  ASSERT_TRUE(cursor->isValid());
}
