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
using kagome::storage::trie::BranchNode;
using kagome::storage::trie::KeyNibbles;
using kagome::storage::trie::PolkadotCodec;
using kagome::storage::trie::PolkadotTrie;
using kagome::storage::trie::PolkadotTrieImpl;
using kagome::storage::trie::TrieNode;

/**
 * Automation of operations over a trie
 */
enum class Command { PUT, REMOVE, GET, CONTAINS };

struct TrieCommand {
  Buffer key;
  std::optional<Buffer> value;
  Command command;
};

class TrieTest
    : public testing::Test,
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
    ASSERT_OUTCOME_SUCCESS_TRY(trie.put(entry.first, entry.second));
  }
}

#define ASSERT_OUTCOME_IS_TRUE(Expression)        \
  {                                               \
    ASSERT_OUTCOME_SUCCESS(result, (Expression)); \
    ASSERT_TRUE(result);                          \
  }
#define ASSERT_OUTCOME_IS_FALSE(Expression)       \
  {                                               \
    ASSERT_OUTCOME_SUCCESS(result, (Expression)); \
    ASSERT_FALSE(result);                         \
  }

/**
 * Runs a sequence of commands provided as a test parameter and checks the
 * result of their execution
 */
TEST_P(TrieTest, RunCommand) {
  for (auto &command : GetParam()) {
    switch (command.command) {
      case Command::CONTAINS:
        ASSERT_OUTCOME_IS_TRUE(trie->contains(command.key));
        break;
      case Command::GET: {
        if (command.value) {
          ASSERT_OUTCOME_SUCCESS(val, trie->get(command.key));
          ASSERT_EQ(val.get(), command.value.value());
        } else {
          EXPECT_OUTCOME_FALSE(err, trie->get(command.key));
          ASSERT_EQ(
              err.value(),
              static_cast<int>(kagome::storage::trie::TrieError::NO_VALUE));
        }
        break;
      }
      case Command::PUT: {
        ASSERT_OUTCOME_SUCCESS_TRY(
            trie->put(command.key, command.value.value()));
        break;
      }
      case Command::REMOVE: {
        ASSERT_OUTCOME_SUCCESS_TRY(trie->remove(command.key));
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
    {"noot"_buf, std::nullopt, Command::GET},
    {"00"_hex2buf, std::nullopt, Command::GET},
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
    {{}, std::nullopt, Command::GET},
    {{}, "floof"_buf, Command::PUT},

    {"09d3"_hex2buf, "noot"_buf, Command::REMOVE},
    {"09d3"_hex2buf, std::nullopt, Command::GET},
    {"0135"_hex2buf, "pen"_buf, Command::GET},
    {"013579"_hex2buf, "penguin"_buf, Command::GET},
    {"09d3"_hex2buf, "noot"_buf, Command::PUT},

    {"f2"_hex2buf, "feather"_buf, Command::REMOVE},
    {"f2"_hex2buf, std::nullopt, Command::GET},
    {"f2"_hex2buf, "feather"_buf, Command::PUT},

    {{}, "floof"_buf, Command::REMOVE},
    {"f2"_hex2buf, "feather"_buf, Command::REMOVE},
    {{}, std::nullopt, Command::GET},
    {"0135"_hex2buf, "pen"_buf, Command::GET},
    {"013579"_hex2buf, "penguin"_buf, Command::GET},
    {{}, "floof"_buf, Command::PUT},
    {"f2"_hex2buf, "feather"_buf, Command::PUT},

    {"013579"_hex2buf, "penguin"_buf, Command::REMOVE},
    {"013579"_hex2buf, std::nullopt, Command::GET},
    {"0135"_hex2buf, "pen"_buf, Command::GET},
    {"013579"_hex2buf, "penguin"_buf, Command::PUT},

    {"0135"_hex2buf, "pen"_buf, Command::REMOVE},
    {"0135"_hex2buf, std::nullopt, Command::GET},
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
    {"09d3"_hex2buf, std::nullopt, Command::GET},
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
    {"0615fc"_hex2buf, std::nullopt, Command::GET},
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
    {"430c"_hex2buf, std::nullopt, Command::GET},
    {"f4bc"_hex2buf, "spaghetti"_buf, Command::PUT},
    {"f4bc"_hex2buf, "spaghetti"_buf, Command::GET},
    {"4f4d"_hex2buf, "stuff"_buf, Command::GET},
    {"43c1"_hex2buf, "noot"_buf, Command::GET}};

INSTANTIATE_TEST_SUITE_P(GolkadotSuite,
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
    ASSERT_OUTCOME_SUCCESS(res, trie->get(entry.first));
    ASSERT_EQ(res.get(), entry.second);
  }
  ASSERT_OUTCOME_SUCCESS_TRY(trie->put("102030"_hex2buf, "0a0b0c"_hex2buf));
  ASSERT_OUTCOME_SUCCESS_TRY(trie->put("104050"_hex2buf, "0a0b0c"_hex2buf));
  ASSERT_OUTCOME_SUCCESS_TRY(trie->put("102030"_hex2buf, "010203"_hex2buf));
  ASSERT_OUTCOME_SUCCESS(v1, trie->get("102030"_hex2buf));
  ASSERT_EQ(v1.get(), "010203"_hex2buf);
  ASSERT_OUTCOME_SUCCESS(v2, trie->get("104050"_hex2buf));
  ASSERT_EQ(v2.get(), "0a0b0c"_hex2buf);
  ASSERT_OUTCOME_SUCCESS_TRY(trie->put("1332"_hex2buf, ""_buf));
  ASSERT_OUTCOME_SUCCESS(v3, trie->get("1332"_hex2buf));
  ASSERT_EQ(v3.get(), ""_buf);
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
    ASSERT_OUTCOME_SUCCESS_TRY(trie->remove(data[i].first));
  }

  for (auto i : {2, 3, 4}) {
    ASSERT_OUTCOME_IS_FALSE(trie->contains(data[i].first));
  }
  ASSERT_OUTCOME_IS_TRUE(trie->contains(data[0].first));
  ASSERT_OUTCOME_IS_TRUE(trie->contains(data[1].first));
}

/**
 * @given a small trie
 * @when replacing an entry in it (put a data with an existing key)
 * @then the value on the key is updated
 */
TEST_F(TrieTest, Replace) {
  FillSmallTree(*trie);

  ASSERT_OUTCOME_SUCCESS_TRY(trie->put(data[1].first, data[3].second));
  ASSERT_OUTCOME_SUCCESS(res, trie->get(data[1].first));
  ASSERT_EQ(res.get(), data[3].second);
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
    ASSERT_OUTCOME_SUCCESS_TRY(trie->put(entry.first, entry.second));
  }
  ASSERT_OUTCOME_SUCCESS_TRY(
      trie->clearPrefix("bar"_buf, std::nullopt, [](const auto &, auto &&) {
        return outcome::success();
      }));
  ASSERT_OUTCOME_IS_TRUE(trie->contains("bat"_buf));
  ASSERT_OUTCOME_IS_TRUE(trie->contains("batch"_buf));
  ASSERT_OUTCOME_IS_FALSE(trie->contains("bark"_buf));
  ASSERT_OUTCOME_IS_FALSE(trie->contains("barnacle"_buf));

  ASSERT_OUTCOME_SUCCESS_TRY(
      trie->clearPrefix("batc"_buf, std::nullopt, [](const auto &, auto &&) {
        return outcome::success();
      }));
  ASSERT_OUTCOME_IS_TRUE(trie->contains("bat"_buf));
  ASSERT_OUTCOME_IS_FALSE(trie->contains("batch"_buf));

  ASSERT_OUTCOME_SUCCESS_TRY(
      trie->clearPrefix("b"_buf, std::nullopt, [](const auto &, auto &&) {
        return outcome::success();
      }));
  ASSERT_OUTCOME_IS_FALSE(trie->contains("bat"_buf));
  ASSERT_TRUE(trie->empty());
}

struct DeleteData {
  std::vector<Buffer> data;
  Buffer key;
  size_t size;
};

class DeleteTest : public testing::Test,
                   public ::testing::WithParamInterface<DeleteData> {
 public:
  DeleteTest() {}

  void SetUp() override {
    trie = std::make_unique<PolkadotTrieImpl>();
  }

  std::unique_ptr<PolkadotTrieImpl> trie;
};

size_t size(const PolkadotTrie::NodePtr &node) {
  size_t count = 0;
  BOOST_ASSERT(node != nullptr);
  if (node->isBranch()) {
    auto branch =
        std::dynamic_pointer_cast<kagome::storage::trie::BranchNode>(node);
    for (const auto &child : branch->children) {
      if (child != nullptr) {
        auto child_node = std::dynamic_pointer_cast<TrieNode>(child);
        count += size(child_node);
      }
    }
  }
  ++count;
  return count;
}

/**
 * @given a trie with entries from DeleteData::data
 * @when removing an entry DeleteData::key
 * @then check key removal by checking tree size equal DeleteData::size
 */
TEST_P(DeleteTest, DeleteData) {
  for (auto &entry : GetParam().data) {
    ASSERT_OUTCOME_SUCCESS_TRY(trie->put(entry, "123"_buf));
  }
  ASSERT_OUTCOME_SUCCESS_TRY(trie->remove(GetParam().key));
  ASSERT_EQ(size(trie->getRoot()), GetParam().size);
}

INSTANTIATE_TEST_SUITE_P(
    DeleteSuite,
    DeleteTest,
    testing::ValuesIn(
        {DeleteData{{}, "bar"_buf, 0},
         DeleteData{{"bar"_buf, "foo"_buf}, "bar"_buf, 1},
         DeleteData{{""_buf, "bar"_buf, "foo"_buf}, "bar"_buf, 2},
         DeleteData{{"bar"_buf, "foa"_buf, "fob"_buf}, "bar"_buf, 3},
         DeleteData{{"612355"_hex2buf, "6124"_hex2buf}, "6123"_hex2buf, 3},
         DeleteData{{"b"_buf, "ba"_buf, "bb"_buf}, "b"_buf, 3},
         DeleteData{{"a"_buf, "b"_buf, "z"_buf}, "z"_buf, 3}}));

struct ClearPrefixData {
  std::vector<Buffer> data;
  Buffer prefix;
  std::optional<uint64_t> limit;
  std::vector<Buffer> res;
  std::tuple<bool, uint32_t> ret;
  size_t size;
};

class ClearPrefixTest : public testing::Test,
                        public ::testing::WithParamInterface<ClearPrefixData> {
 public:
  ClearPrefixTest() {}

  void SetUp() override {
    trie = std::make_unique<PolkadotTrieImpl>();
  }

  std::unique_ptr<PolkadotTrieImpl> trie;
};

/**
 * @given a trie with entries from ClearPrefixData::data
 * @when deleting entries in it that start with a prefix ClearPrefixData::prefix
 * with limit set to ClearPrefixData::limit
 * @then then check that trie has all values from ClearPrefixData::res, has size
 * ClearPrefixData::size, and returns ClearPrefixData::ret
 */
TEST_P(ClearPrefixTest, ManyCases) {
  for (const auto &entry : GetParam().data) {
    ASSERT_OUTCOME_SUCCESS_TRY(trie->put(entry, "123"_buf));
  }
  ASSERT_OUTCOME_SUCCESS(
      ret,
      trie->clearPrefix(
          GetParam().prefix, GetParam().limit, [](const auto &, auto &&) {
            return outcome::success();
          }));
  EXPECT_EQ(ret, GetParam().ret);
  for (const auto &entry : GetParam().res) {
    ASSERT_OUTCOME_IS_TRUE(trie->contains(entry));
  }
  ASSERT_EQ(size(trie->getRoot()), GetParam().size);
}

INSTANTIATE_TEST_SUITE_P(
    ClearPrefixSuite,
    ClearPrefixTest,
    testing::ValuesIn(
        /* empty tree */
        {ClearPrefixData{{}, "bar"_buf, std::nullopt, {}, {true, 0}, 0},
         /* miss */
         ClearPrefixData{
             {"bo"_buf}, "agu"_buf, std::nullopt, {"bo"_buf}, {true, 0}, 1},
         /* equal start but no children */
         ClearPrefixData{
             {"bo"_buf}, "boo"_buf, std::nullopt, {"bo"_buf}, {true, 0}, 1},
         /* prefix matches leaf */
         ClearPrefixData{{"bar"_buf, "foo"_buf},
                         "bar"_buf,
                         std::nullopt,
                         {"foo"_buf},
                         {true, 1},
                         1},
         /* empty prefix */
         ClearPrefixData{
             {"bar"_buf, "foo"_buf}, ""_buf, std::nullopt, {}, {true, 2}, 0},
         /* "b"-node converts to leaf */
         ClearPrefixData{{"a"_buf, "b"_buf, "boa"_buf, "bob"_buf},
                         "bo"_buf,
                         std::nullopt,
                         {"a"_buf, "b"_buf},
                         {true, 2},
                         3},
         /* "b"-node becomes "ba"-node */
         ClearPrefixData{{"a"_buf, "baa"_buf, "bab"_buf, "boa"_buf, "bob"_buf},
                         "bo"_buf,
                         std::nullopt,
                         {"a"_buf, "baa"_buf, "bab"_buf},
                         {true, 2},
                         5},
         /* a limit to remove all */
         ClearPrefixData{
             {"a"_buf, "b"_buf, "c"_buf}, ""_buf, 3, {}, {true, 3}, 0},
         /* remove from wide tree, stop by limit */
         ClearPrefixData{
             {"a"_buf, "b"_buf, "c"_buf}, ""_buf, 2, {"c"_buf}, {false, 2}, 1},
         /* remove from long tree, stop by limit */
         ClearPrefixData{{""_buf, "a"_buf, "aa"_buf, "aaa"_buf},
                         "a"_buf,
                         2,
                         {""_buf, "a"_buf},
                         {false, 2},
                         2}}));

/**
 * @given an empty trie
 * @when putting something into the trie
 * @then the trie is empty no more
 */
TEST_F(TrieTest, EmptyTrie) {
  ASSERT_TRUE(trie->empty());
  ASSERT_OUTCOME_SUCCESS_TRY(trie->put(Buffer{0}, "asdasd"_buf));
  ASSERT_FALSE(trie->empty());
}

/**
 * @given a trie
 * @when getting a path in a trie to a valid node
 * @then the path is returned
 */
TEST_F(TrieTest, GetPath) {
  // TODO(Harrm) PRE-461 Make parametrized
  const std::vector<std::pair<Buffer, Buffer>> data = {
      {"123456"_hex2buf, "42"_hex2buf},
      {"1234"_hex2buf, "1234"_hex2buf},
      {"010203"_hex2buf, "0a0b"_hex2buf},
      {"010a0b"_hex2buf, "1337"_hex2buf},
      {"0a0b0c"_hex2buf, "deadbeef"_hex2buf}};

  for (const auto &entry : TrieTest::data) {
    ASSERT_OUTCOME_SUCCESS_TRY(trie->put(entry.first, entry.second));
  }

  std::vector<std::pair<const BranchNode *, uint8_t>> path;
  ASSERT_OUTCOME_SUCCESS_TRY(
      trie->forNodeInPath(trie->getRoot(),
                          KeyNibbles{"010203040506"_hex2buf},
                          [&path](const auto &node, auto idx) mutable {
                            path.emplace_back(&node, idx);
                            return outcome::success();
                          }))
  auto root = trie->getRoot();
  auto node1 = trie->getNode(root, KeyNibbles{1, 2, 3, 4}).value();
  auto it = path.begin();
  ASSERT_EQ(it->first, root.get());
  ASSERT_EQ(it->first->children[it->second], node1);
  ASSERT_EQ((++it)->first, node1.get());
}

/**
 * @given a trie
 * @when getting a path in a trie to a non-existing node
 * @then an error is returned
 */
TEST_F(TrieTest, GetPathToInvalid) {
  const std::vector<std::pair<Buffer, Buffer>> data = {
      {"123456"_hex2buf, "42"_hex2buf},
      {"1234"_hex2buf, "1234"_hex2buf},
      {"010203"_hex2buf, "0a0b"_hex2buf},
      {"010a0b"_hex2buf, "1337"_hex2buf},
      {"0a0b0c"_hex2buf, "deadbeef"_hex2buf}};

  for (const auto &entry : TrieTest::data) {
    ASSERT_OUTCOME_SUCCESS_TRY(trie->put(entry.first, entry.second));
  }
  EXPECT_OUTCOME_SOME_ERROR(
      _,
      trie->forNodeInPath(
          trie->getRoot(),
          KeyNibbles{"0a0b0c0d0e0f"_hex2buf},
          [](auto &node, auto idx) mutable { return outcome::success(); }))
}

/**
 * @given a trie
 * @when searching with getNode() for a non present key
 * @then nullptr is returned
 */
TEST_F(TrieTest, GetNodeReturnsNullptrWhenNotFound) {
  const std::vector<std::pair<Buffer, Buffer>> data = {
      {"123456"_hex2buf, "42"_hex2buf},
      {"1234"_hex2buf, "1234"_hex2buf},
      {"010203"_hex2buf, "0a0b"_hex2buf},
      {"010a0b"_hex2buf, "1337"_hex2buf},
      {"0a0b0c"_hex2buf, "deadbeef"_hex2buf}};

  for (auto &entry : TrieTest::data) {
    ASSERT_OUTCOME_SUCCESS_TRY(trie->put(entry.first, entry.second));
  }
  ASSERT_OUTCOME_SUCCESS(
      res,
      trie->getNode(trie->getRoot(), KeyNibbles{"01020304050607"_hex2buf}));
  ASSERT_EQ(res, nullptr) << res->value->toHex();
}
