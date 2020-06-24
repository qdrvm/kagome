/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::storage::trie::PolkadotCodec;
using kagome::storage::trie::PolkadotTrie;
using kagome::storage::trie::PolkadotTrieImpl;

/**
 * Automation of operations over a trie
 */
enum class Command { PUT, REMOVE, GET, CONTAINS };

struct TrieCommand {
  Buffer key;
  boost::optional<Buffer> value;
  Command command;
};

class TrieTest: public testing::Test,
      public ::testing::WithParamInterface<std::vector<TrieCommand>> {
 public:
  TrieTest() {}

  void SetUp() override {
    trie = std::make_unique<PolkadotTrieImpl>();
  }

  static const std::vector<std::pair<Buffer, Buffer>> data;

  std::unique_ptr<PolkadotTrieImpl> trie;
};

const std::vector<std::pair<Buffer, Buffer>> TrieTest::data = {
    {"123456"_hex2buf, "42"_hex2buf},
    {"1234"_hex2buf, "1234"_hex2buf},
    {"010203"_hex2buf, "0a0b"_hex2buf},
    {"010a0b"_hex2buf, "1337"_hex2buf},
    {"0a0b0c"_hex2buf, "deadbeef"_hex2buf}};

void FillSmallTree(PolkadotTrie &trie) {
  for (auto &entry : TrieTest::data) {
    EXPECT_OUTCOME_TRUE_1(trie.put(entry.first, entry.second));
  }
}

/**
 * Runs a sequence of commands provided as a test parameter and checks the
 * result of their execution
 */
TEST_P(TrieTest, RunCommand) {
  for (auto &command : GetParam()) {
    switch (command.command) {
      case Command::CONTAINS:
        ASSERT_TRUE(trie->contains(command.key));
        break;
      case Command::GET: {
        if (command.value) {
          EXPECT_OUTCOME_TRUE(val, trie->get(command.key));
          ASSERT_EQ(val, command.value.get());
        } else {
          EXPECT_OUTCOME_FALSE(err, trie->get(command.key));
          ASSERT_EQ(
              err.value(),
              static_cast<int>(kagome::storage::trie::TrieError::NO_VALUE));
        }
        break;
      }
      case Command::PUT: {
        EXPECT_OUTCOME_TRUE_1(trie->put(command.key, command.value.get()));
        break;
      }
      case Command::REMOVE: {
        EXPECT_OUTCOME_TRUE_1(trie->remove(command.key));
        break;
      }
    }
  }
}

template <typename T>
inline std::vector<T> concat(const std::vector<T> &v1,
                             const std::vector<T> &v2) {
  std::vector<T> v(v1);
  v.insert(v.end(), v2.begin(), v2.end());
  return v;
}

/**
 * Create a small trie with one branch and several leaves
 */
std::vector<TrieCommand> BuildSmallTrie = {
    {"0135"_hex2buf, "pen"_buf, Command::PUT},
    {"013579"_hex2buf, "penguin"_buf, Command::PUT},
    {"f2"_hex2buf, "feather"_buf, Command::PUT},
    {"09d3"_hex2buf, "noot"_buf, Command::PUT},
    {Buffer{}, "floof"_buf, Command::PUT},
    {"013507"_hex2buf, "odd"_buf, Command::PUT}};
/**
 * Create a tree with a branch and check that every inserted value is
 * accessible
 */
std::vector<TrieCommand> PutAndGetBranch = {
    {"0135"_hex2buf, ("spaghetti"_buf), Command::PUT},
    {"013579"_hex2buf, ("gnocchi"_buf), Command::PUT},
    {"07"_hex2buf, ("ramen"_buf), Command::PUT},
    {"f2"_hex2buf, ("pho"_buf), Command::PUT},
    {"noot"_buf, boost::none, Command::GET},
    {"00"_hex2buf, boost::none, Command::GET},
    {"0135"_hex2buf, ("spaghetti"_buf), Command::GET},
    {"013579"_hex2buf, ("gnocchi"_buf), Command::GET},
    {"07"_hex2buf, ("ramen"_buf), Command::GET},
    {"f2"_hex2buf, ("pho"_buf), Command::GET}};
/**
 * As key is decomposed to nibbles (4 bit pieces), odd length might be
 * processed incorrectly, which is checked here
 */
std::vector<TrieCommand> PutAndGetOddKeyLengths = {
    {"43c1"_hex2buf, "noot"_buf, Command::PUT},
    {"4929"_hex2buf, "nootagain"_buf, Command::PUT},
    {"430c"_hex2buf, "odd"_buf, Command::PUT},
    {"4f4d"_hex2buf, "stuff"_buf, Command::PUT},
    {"4fbc"_hex2buf, "stuffagain"_buf, Command::PUT},
    {"43c1"_hex2buf, "noot"_buf, Command::GET},
    {"4929"_hex2buf, "nootagain"_buf, Command::GET},
    {"430c"_hex2buf, "odd"_buf, Command::GET},
    {"4f4d"_hex2buf, "stuff"_buf, Command::GET},
    {"4fbc"_hex2buf, "stuffagain"_buf, Command::GET}};
/**
 * Deletion from a small trie. BuildSmallTrie must be applied before this case
 */
std::vector<TrieCommand> DeleteSmall = {
    {{}, "floof"_buf, Command::REMOVE},
    {{}, boost::none, Command::GET},
    {{}, "floof"_buf, Command::PUT},

    {"09d3"_hex2buf, "noot"_buf, Command::REMOVE},
    {"09d3"_hex2buf, boost::none, Command::GET},
    {"0135"_hex2buf, "pen"_buf, Command::GET},
    {"013579"_hex2buf, "penguin"_buf, Command::GET},
    {"09d3"_hex2buf, "noot"_buf, Command::PUT},

    {"f2"_hex2buf, "feather"_buf, Command::REMOVE},
    {"f2"_hex2buf, boost::none, Command::GET},
    {"f2"_hex2buf, "feather"_buf, Command::PUT},

    {{}, "floof"_buf, Command::REMOVE},
    {"f2"_hex2buf, "feather"_buf, Command::REMOVE},
    {{}, boost::none, Command::GET},
    {"0135"_hex2buf, "pen"_buf, Command::GET},
    {"013579"_hex2buf, "penguin"_buf, Command::GET},
    {{}, "floof"_buf, Command::PUT},
    {"f2"_hex2buf, "feather"_buf, Command::PUT},

    {"013579"_hex2buf, "penguin"_buf, Command::REMOVE},
    {"013579"_hex2buf, boost::none, Command::GET},
    {"0135"_hex2buf, "pen"_buf, Command::GET},
    {"013579"_hex2buf, "penguin"_buf, Command::PUT},

    {"0135"_hex2buf, "pen"_buf, Command::REMOVE},
    {"0135"_hex2buf, boost::none, Command::GET},
    {"013579"_hex2buf, "penguin"_buf, Command::GET},
    {"0135"_hex2buf, "pen"_buf, Command::PUT},

    {"013507"_hex2buf, "odd"_buf, Command::REMOVE},
    {"013579"_hex2buf, "penguin"_buf, Command::GET},
    {"0135"_hex2buf, "pen"_buf, Command::GET},
};
/**
 * Deletion from a complex branch. BuildSmallTrie must be applied before this
 * suit
 */
std::vector<TrieCommand> DeleteCombineBranch = {
    {"013546"_hex2buf, "raccoon"_buf, Command::PUT},
    {"01354677"_hex2buf, "rat"_buf, Command::PUT},
    {"09d3"_hex2buf, "noot"_buf, Command::REMOVE},
    {"09d3"_hex2buf, boost::none, Command::GET},
};
/**
 * Deletion from a branch
 */
std::vector<TrieCommand> DeleteFromBranch = {
    {"0615fc"_hex2buf, "noot"_buf, Command::PUT},
    {"062ba9"_hex2buf, "nootagain"_buf, Command::PUT},
    {"06afb1"_hex2buf, "odd"_buf, Command::PUT},
    {"06a3ff"_hex2buf, "stuff"_buf, Command::PUT},
    {"4321"_hex2buf, "stuffagain"_buf, Command::PUT},
    {"0615fc"_hex2buf, "noot"_buf, Command::GET},
    {"062ba9"_hex2buf, "nootagain"_buf, Command::GET},
    {"0615fc"_hex2buf, "noot"_buf, Command::REMOVE},
    {"0615fc"_hex2buf, boost::none, Command::GET},
    {"062ba9"_hex2buf, "nootagain"_buf, Command::GET},
    {"06afb1"_hex2buf, "odd"_buf, Command::GET},
    {"06afb1"_hex2buf, "odd"_buf, Command::REMOVE},
    {"062ba9"_hex2buf, "nootagain"_buf, Command::GET},
    {"06a3ff"_hex2buf, "stuff"_buf, Command::GET},
    {"06a3ff"_hex2buf, "stuff"_buf, Command::REMOVE},
    {"062ba9"_hex2buf, "nootagain"_buf, Command::GET}};
/**
 * Deletion on keys with odd length, which might be a problem as a key is
 * decomposed to 4 bit pieces
 */
std::vector<TrieCommand> DeleteOddKeyLengths = {
    {"43c1"_hex2buf, "noot"_buf, Command::PUT},
    {"43c1"_hex2buf, "noot"_buf, Command::GET},
    {"4929"_hex2buf, "nootagain"_buf, Command::PUT},
    {"4929"_hex2buf, "nootagain"_buf, Command::GET},
    {"430c"_hex2buf, "odd"_buf, Command::PUT},
    {"430c"_hex2buf, "odd"_buf, Command::GET},
    {"4f4d"_hex2buf, "stuff"_buf, Command::PUT},
    {"4f4d"_hex2buf, "stuff"_buf, Command::GET},
    {"430c"_hex2buf, "odd"_buf, Command::REMOVE},
    {"430c"_hex2buf, boost::none, Command::GET},
    {"f4bc"_hex2buf, "spaghetti"_buf, Command::PUT},
    {"f4bc"_hex2buf, "spaghetti"_buf, Command::GET},
    {"4f4d"_hex2buf, "stuff"_buf, Command::GET},
    {"43c1"_hex2buf, "noot"_buf, Command::GET}};

INSTANTIATE_TEST_CASE_P(GolkadotSuite,
                        TrieTest,
                        testing::ValuesIn({PutAndGetBranch,
                                           PutAndGetOddKeyLengths,
                                           concat(BuildSmallTrie, DeleteSmall),
                                           concat(BuildSmallTrie,
                                                  DeleteCombineBranch),
                                           DeleteFromBranch,
                                           DeleteOddKeyLengths}));

/**
 * @given an empty trie
 * @when putting some entries into it
 * @then all inserted entries are accessible from the trie
 */
TEST_F(TrieTest, Put) {
  FillSmallTree(*trie);

  for (auto &entry : data) {
    EXPECT_OUTCOME_TRUE(res, trie->get(entry.first));
    ASSERT_EQ(res, entry.second);
  }
  EXPECT_OUTCOME_TRUE_1(trie->put("102030"_hex2buf, "0a0b0c"_hex2buf));
  EXPECT_OUTCOME_TRUE_1(trie->put("104050"_hex2buf, "0a0b0c"_hex2buf));
  EXPECT_OUTCOME_TRUE_1(trie->put("102030"_hex2buf, "010203"_hex2buf));
  EXPECT_OUTCOME_TRUE(v1, trie->get("102030"_hex2buf));
  ASSERT_EQ(v1, "010203"_hex2buf);
  EXPECT_OUTCOME_TRUE(v2, trie->get("104050"_hex2buf));
  ASSERT_EQ(v2, "0a0b0c"_hex2buf);
  EXPECT_OUTCOME_TRUE_1(trie->put("1332"_hex2buf, ""_buf));
  EXPECT_OUTCOME_TRUE(v3, trie->get("1332"_hex2buf));
  ASSERT_EQ(v3, ""_buf);
}

/**
 * @given a small trie
 * @when removing some entries from it
 * @then removed entries are no longer in the trie, while the rest of them
 * stays
 */
TEST_F(TrieTest, Remove) {
  FillSmallTree(*trie);

  for (auto i : {2, 3, 4}) {
    EXPECT_OUTCOME_TRUE_1(trie->remove(data[i].first));
  }

  for (auto i : {2, 3, 4}) {
    ASSERT_FALSE(trie->contains(data[i].first));
  }
  ASSERT_TRUE(trie->contains(data[0].first));
  ASSERT_TRUE(trie->contains(data[1].first));
}

/**
 * @given a small trie
 * @when replacing an entry in it (put a data with an existing key)
 * @then the value on the key is updated
 */
TEST_F(TrieTest, Replace) {
  FillSmallTree(*trie);

  EXPECT_OUTCOME_TRUE_1(trie->put(data[1].first, data[3].second));
  EXPECT_OUTCOME_TRUE(res, trie->get(data[1].first));
  ASSERT_EQ(res, data[3].second);
}

/**
 * @given a trie
 * @when deleting entries in it that start with a prefix
 * @then there is no entries with such prefix in the trie
 */
TEST_F(TrieTest, ClearPrefix) {
  std::vector<std::pair<Buffer, Buffer>> data = {{"bark"_buf, "123"_buf},
                                                 {"barnacle"_buf, "456"_buf},
                                                 {"bat"_buf, "789"_buf},
                                                 {"batch"_buf, "0-="_buf}};
  for (auto &entry : data) {
    EXPECT_OUTCOME_TRUE_1(trie->put(entry.first, entry.second));
  }
  EXPECT_OUTCOME_TRUE_1(trie->clearPrefix("bar"_buf));
  ASSERT_TRUE(trie->contains("bat"_buf));
  ASSERT_TRUE(trie->contains("batch"_buf));
  ASSERT_FALSE(trie->contains("bark"_buf));
  ASSERT_FALSE(trie->contains("barnacle"_buf));

  EXPECT_OUTCOME_TRUE_1(trie->clearPrefix("batc"_buf));
  ASSERT_TRUE(trie->contains("bat"_buf));
  ASSERT_FALSE(trie->contains("batch"_buf));

  EXPECT_OUTCOME_TRUE_1(trie->clearPrefix("b"_buf));
  ASSERT_FALSE(trie->contains("bat"_buf));
  ASSERT_TRUE(trie->empty());
}

/**
 * @given an empty trie
 * @when putting something into the trie
 * @then the trie is empty no more
 */
TEST_F(TrieTest, EmptyTrie) {
  ASSERT_TRUE(trie->empty());
  EXPECT_OUTCOME_TRUE_1(trie->put({0}, "asdasd"_buf));
  ASSERT_FALSE(trie->empty());
}
