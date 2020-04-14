/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/polkadot_trie_db.hpp"
#include "storage/trie/impl/trie_db_backend_impl.hpp"
#include "storage/trie/impl/trie_error.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_leveldb_test.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::storage::LevelDB;
using kagome::storage::trie::PolkadotTrieDb;
using kagome::storage::trie::TrieDbBackendImpl;

static const Buffer kNodePrefix{1};

/**
 * Automation of operations over a trie
 */
enum class Command { PUT, REMOVE, GET, CONTAINS };

struct TrieCommand {
  Buffer key;
  boost::optional<Buffer> value;
  Command command;
};

class TrieTest
    : public test::BaseLevelDB_Test,
      public ::testing::WithParamInterface<std::vector<TrieCommand>> {
 public:
  TrieTest() : BaseLevelDB_Test("/tmp/leveldb_test") {}

  void SetUp() override {
    open();
    trie = PolkadotTrieDb::createEmpty(std::make_shared<TrieDbBackendImpl>(db_, kNodePrefix));
  }

  static const std::vector<std::pair<Buffer, Buffer>> data;

  std::unique_ptr<PolkadotTrieDb> trie;
};

const std::vector<std::pair<Buffer, Buffer>> TrieTest::data = {
    {"123456"_hex2buf, "42"_hex2buf},
    {"1234"_hex2buf, "1234"_hex2buf},
    {"010203"_hex2buf, "0a0b"_hex2buf},
    {"010a0b"_hex2buf, "1337"_hex2buf},
    {"0a0b0c"_hex2buf, "deadbeef"_hex2buf}};

void FillSmallTree(PolkadotTrieDb &trie) {
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

/**
 * @given an empty persistent trie with LevelDb backend
 * @when putting a value into it @and its intance is destroyed @and a new
 * instance initialsed with the same DB
 * @then the new instance contains the same data
 */
TEST_F(TrieTest, CreateDestroyCreate) {
  Buffer root;
  {
    leveldb::Options options;
    options.create_if_missing = true;  // intentionally
    EXPECT_OUTCOME_TRUE(
        level_db,
        LevelDB::create("/tmp/kagome_leveldb_persistency_test", options));
    auto db = PolkadotTrieDb::createEmpty(
        std::make_shared<TrieDbBackendImpl>(std::move(level_db), kNodePrefix));

    // case 1
    EXPECT_OUTCOME_TRUE_1(db->put("3a65787472696e7369635f696e646578"_hex2buf,
                                  "00000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("8cb577756012d928f17362e0741f9f2c"_hex2buf,
                                  "03000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "f7787e54bb33faaf40a7f3bf438458ee"_hex2buf,
        "040642414245e1010000000000000000001400000000000000010000008076f2a08296f25d55febf7acbd962ee5c70bd94d5f7a71679a939a59c44b027150101668336e93123a070b9bfa139bcf3a82882321383cdb347f74e21ee7c9ed5690fbd03cfa00da52ed3800bcb91b396b06d68065748911d12c7ddcdf0bd95de8603"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "3ae31af9a378162eb2736f26855c9ad8"_hex2buf,
        "ebd7c27da1aa712f1ff449e04f8975cc2358beba8513da6d581e66d858424960"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "224bd05121db69c8dcf92845e276ccc44350c1a65a3f9aca9fd0734370b5870a"_hex2buf,
        "ebd7c27da1aa712f1ff449e04f8975cc2358beba8513da6d581e66d858424960"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "e1856c2284210a998e3257b3d8a315db"_hex2buf,
        "0000000000000000000000000000000000000000000000000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("cc956bdb7605e3547539f321ac2bc95c"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("2e37ecea13014576b46e0b7a6139cbd2"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->remove(
        "3007c9ff7027f65900abcdfca4fdb107ead47e2a9e3558e01b691b0f4a5f8518"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->put("e0410aa8e1aff5af1147fe2f9b33ce62"_hex2buf, "00"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->put("234b134360b60e7713043dc1ea9a752e"_hex2buf, "00"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("9ef8d3fecf9615ad693470693c7fb7dd"_hex2buf,
                                  "0000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "306489973b594fdfe42d5055b60b70134f81d3291fed263236bc07b94fc6d3ea89eb0d6a8a691dae2cd15ed0369931ce0a949ecafa5c3f93f8121833646e15c3"_hex2buf,
        "03000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "ca263a1d57613bec1f68af5eb50a2d31"_hex2buf,
        "0cb5ebfaf1fb6560d20e30a772c5482affeb5955602062a550b326b2e7135bb7a486b7543c1945f4e0856263d8ec56ef3b3ef22e3bef8ac71e6caa962b3b4705b5ebd7c27da1aa712f1ff449e04f8975cc2358beba8513da6d581e66d858424960"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "4fbfbfa5fc2e8c0a7265bcb04f86338f004320a0c2ed9d66fcee8e68b7595b7b"_hex2buf,
        "2c280403000b60a270747101"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("0e4944cfd98d6f4cc374d16f5a4e3f9c"_hex2buf,
                                  "60a2707471010000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->put("052cfee4ed51fa512e9fed56a4185f26"_hex2buf, "01"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("2e37ecea13014576b46e0b7a6139cbd2"_hex2buf,
                                  "01000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("cc956bdb7605e3547539f321ac2bc95c"_hex2buf,
                                  "040000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("3a65787472696e7369635f696e646578"_hex2buf,
                                  "01000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "bfcf5ee1dae60fc10ebd79b603ced5d0546b9143f775294c18cd14e2cc2fcc34"_hex2buf,
        "3102290284ffdc3488acc1a6b90aa92cea0cfbe2b00754a74084970b08d968e948d4d3bf161a01e2f2be0a634faeb8401ed2392731df803877dcb2422bb396d48ca24f18661059e3dde41d14b87eb929ec41ab36e6d63be5a1f5c3c5c092c79646a453f4b392890000000600ff488f6d1b0114674dcd81fd29642bc3bcec8c8366f6af0665860f9d4e8c8a972404"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "6dc6083a34f64d6836b8deff5623799f03bb1a4d5e60d73680c1f68297a69c3b"_hex2buf,
        "01000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("94310e87b78915dd7e2656ce42bda477"_hex2buf,
                                  "8c000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("6301b8a945c24ad6ef0fd5d063c34f19"_hex2buf,
                                  "40420f00"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "8cf1a75ecd7e045ad962dc579b2aeb09efd5ca7d8f654eed7c45f9c85d031a58"_hex2buf,
        "4060d8d667a9c9353600000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "ba627d728bc02c61b22a8ee2fb84dc3c7edf5a61b45ee13e8802329c8e9d4acc"_hex2buf,
        "00f3197d715e00000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("78f4ad73d6b7279f8d06f359e363c829"_hex2buf,
                                  "a04947913b8fed353600000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("2e37ecea13014576b46e0b7a6139cbd2"_hex2buf,
                                  "02000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "cc956bdb7605e3547539f321ac2bc95c"_hex2buf,
        "08000000000000000000010000000c0580d94f36bf010000000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("78f4ad73d6b7279f8d06f359e363c829"_hex2buf,
                                  "4053b3c3cb8eed353600000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "8cf1a75ecd7e045ad962dc579b2aeb09efd5ca7d8f654eed7c45f9c85d031a58"_hex2buf,
        "3f5033027fa8c9353600000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "b9c63c80ee65dceb452dc1d3d1ba07108e2cca06b0d914af112a6c34c41d8d10"_hex2buf,
        "0100c16ff28623000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("78f4ad73d6b7279f8d06f359e363c829"_hex2buf,
                                  "40430eefe28ded353600000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("2e37ecea13014576b46e0b7a6139cbd2"_hex2buf,
                                  "03000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "cc956bdb7605e3547539f321ac2bc95c"_hex2buf,
        "0c000000000000000000010000000c0580d94f36bf01000000000000000000000000010000000302dc3488acc1a6b90aa92cea0cfbe2b00754a74084970b08d968e948d4d3bf161a488f6d1b0114674dcd81fd29642bc3bcec8c8366f6af0665860f9d4e8c8a9724010000000000000000000000000000000010a5d4e8000000000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("2e37ecea13014576b46e0b7a6139cbd2"_hex2buf,
                                  "04000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "cc956bdb7605e3547539f321ac2bc95c"_hex2buf,
        "10000000000000000000010000000c0580d94f36bf01000000000000000000000000010000000302dc3488acc1a6b90aa92cea0cfbe2b00754a74084970b08d968e948d4d3bf161a488f6d1b0114674dcd81fd29642bc3bcec8c8366f6af0665860f9d4e8c8a9724010000000000000000000000000000000010a5d4e80000000000000000000000000001000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("3a65787472696e7369635f696e646578"_hex2buf,
                                  "02000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("3a65787472696e7369635f696e646578"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("77f9812b2b420bfcce4391ad7ae9451b"_hex2buf,
                                  "02000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("e0410aa8e1aff5af1147fe2f9b33ce62"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("052cfee4ed51fa512e9fed56a4185f26"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("a85394678161136a273c54d7fe328306"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("234b134360b60e7713043dc1ea9a752e"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("9bebe08756f425a9502ee26032757a6a"_hex2buf,
                                  "f88affffffffffff"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->put("9091b72da6974ffc45483e63e74b74fd"_hex2buf,
                "1000000000000000000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->put("0e348355f6bfaf32d8e947e896fe3536"_hex2buf,
                "1000000000000000000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("0deef7973bb5154589f1438f1f21d3a1"_hex2buf,
                                  "00000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("1c9f2b74227c8236327fe66693641e42"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->remove(
        "4fbfbfa5fc2e8c0a7265bcb04f86338f004320a0c2ed9d66fcee8e68b7595b7b"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->remove(
        "bfcf5ee1dae60fc10ebd79b603ced5d0546b9143f775294c18cd14e2cc2fcc34"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "e1856c2284210a998e3257b3d8a315db"_hex2buf,
        "03bfaeedc1512007f92ef10b0345bc0a1c253efa9855ee95953fee296163a283"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("77f9812b2b420bfcce4391ad7ae9451b"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("6301b8a945c24ad6ef0fd5d063c34f19"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("94310e87b78915dd7e2656ce42bda477"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("8cb577756012d928f17362e0741f9f2c"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("3ae31af9a378162eb2736f26855c9ad8"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("f7787e54bb33faaf40a7f3bf438458ee"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("e1856c2284210a998e3257b3d8a315db"_hex2buf));

    auto root1 = db->getRootHash();

    // case 2
    db = PolkadotTrieDb::createEmpty(
        std::make_shared<TrieDbBackendImpl>(std::move(db_), kNodePrefix));

    EXPECT_OUTCOME_TRUE_1(db->put("3a65787472696e7369635f696e646578"_hex2buf,
                                  "00000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("8cb577756012d928f17362e0741f9f2c"_hex2buf,
                                  "03000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "f7787e54bb33faaf40a7f3bf438458ee"_hex2buf,
        "040642414245e1010000000000000000001400000000000000010000008076f2a08296f25d55febf7acbd962ee5c70bd94d5f7a71679a939a59c44b027150101668336e93123a070b9bfa139bcf3a82882321383cdb347f74e21ee7c9ed5690fbd03cfa00da52ed3800bcb91b396b06d68065748911d12c7ddcdf0bd95de8603"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "3ae31af9a378162eb2736f26855c9ad8"_hex2buf,
        "ebd7c27da1aa712f1ff449e04f8975cc2358beba8513da6d581e66d858424960"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "224bd05121db69c8dcf92845e276ccc44350c1a65a3f9aca9fd0734370b5870a"_hex2buf,
        "ebd7c27da1aa712f1ff449e04f8975cc2358beba8513da6d581e66d858424960"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "e1856c2284210a998e3257b3d8a315db"_hex2buf,
        "03bfaeedc1512007f92ef10b0345bc0a1c253efa9855ee95953fee296163a283"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("cc956bdb7605e3547539f321ac2bc95c"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("2e37ecea13014576b46e0b7a6139cbd2"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->put("e0410aa8e1aff5af1147fe2f9b33ce62"_hex2buf, "00"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->put("234b134360b60e7713043dc1ea9a752e"_hex2buf, "00"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("9ef8d3fecf9615ad693470693c7fb7dd"_hex2buf,
                                  "0000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "306489973b594fdfe42d5055b60b70134f81d3291fed263236bc07b94fc6d3ea89eb0d6a8a691dae2cd15ed0369931ce0a949ecafa5c3f93f8121833646e15c3"_hex2buf,
        "03000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "ca263a1d57613bec1f68af5eb50a2d31"_hex2buf,
        "0cb5ebfaf1fb6560d20e30a772c5482affeb5955602062a550b326b2e7135bb7a486b7543c1945f4e0856263d8ec56ef3b3ef22e3bef8ac71e6caa962b3b4705b5ebd7c27da1aa712f1ff449e04f8975cc2358beba8513da6d581e66d858424960"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("0e4944cfd98d6f4cc374d16f5a4e3f9c"_hex2buf,
                                  "60a2707471010000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->put("052cfee4ed51fa512e9fed56a4185f26"_hex2buf, "01"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("2e37ecea13014576b46e0b7a6139cbd2"_hex2buf,
                                  "01000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("cc956bdb7605e3547539f321ac2bc95c"_hex2buf,
                                  "040000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("3a65787472696e7369635f696e646578"_hex2buf,
                                  "01000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "6dc6083a34f64d6836b8deff5623799f03bb1a4d5e60d73680c1f68297a69c3b"_hex2buf,
        "01000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("94310e87b78915dd7e2656ce42bda477"_hex2buf,
                                  "8c000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("6301b8a945c24ad6ef0fd5d063c34f19"_hex2buf,
                                  "40420f00"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "8cf1a75ecd7e045ad962dc579b2aeb09efd5ca7d8f654eed7c45f9c85d031a58"_hex2buf,
        "2030bcda96abc9353600000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "ba627d728bc02c61b22a8ee2fb84dc3c7edf5a61b45ee13e8802329c8e9d4acc"_hex2buf,
        "8019ca46b25c00000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("78f4ad73d6b7279f8d06f359e363c829"_hex2buf,
                                  "0040db5eab8fed353600000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("2e37ecea13014576b46e0b7a6139cbd2"_hex2buf,
                                  "02000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "cc956bdb7605e3547539f321ac2bc95c"_hex2buf,
        "08000000000000000000010000000c0580d94f36bf010000000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("78f4ad73d6b7279f8d06f359e363c829"_hex2buf,
                                  "a04947913b8fed353600000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "8cf1a75ecd7e045ad962dc579b2aeb09efd5ca7d8f654eed7c45f9c85d031a58"_hex2buf,
        "1f201706aeaac9353600000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "b9c63c80ee65dceb452dc1d3d1ba07108e2cca06b0d914af112a6c34c41d8d10"_hex2buf,
        "0100c16ff28623000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("78f4ad73d6b7279f8d06f359e363c829"_hex2buf,
                                  "a039a2bc528eed353600000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("2e37ecea13014576b46e0b7a6139cbd2"_hex2buf,
                                  "03000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "cc956bdb7605e3547539f321ac2bc95c"_hex2buf,
        "0c000000000000000000010000000c0580d94f36bf01000000000000000000000000010000000302dc3488acc1a6b90aa92cea0cfbe2b00754a74084970b08d968e948d4d3bf161a488f6d1b0114674dcd81fd29642bc3bcec8c8366f6af0665860f9d4e8c8a9724010000000000000000000000000000000010a5d4e8000000000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("2e37ecea13014576b46e0b7a6139cbd2"_hex2buf,
                                  "04000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put(
        "cc956bdb7605e3547539f321ac2bc95c"_hex2buf,
        "10000000000000000000010000000c0580d94f36bf01000000000000000000000000010000000302dc3488acc1a6b90aa92cea0cfbe2b00754a74084970b08d968e948d4d3bf161a488f6d1b0114674dcd81fd29642bc3bcec8c8366f6af0665860f9d4e8c8a9724010000000000000000000000000000000010a5d4e80000000000000000000000000001000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("3a65787472696e7369635f696e646578"_hex2buf,
                                  "02000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("3a65787472696e7369635f696e646578"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("77f9812b2b420bfcce4391ad7ae9451b"_hex2buf,
                                  "02000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("e0410aa8e1aff5af1147fe2f9b33ce62"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("052cfee4ed51fa512e9fed56a4185f26"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("a85394678161136a273c54d7fe328306"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("234b134360b60e7713043dc1ea9a752e"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("9bebe08756f425a9502ee26032757a6a"_hex2buf,
                                  "f88affffffffffff"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->put("9091b72da6974ffc45483e63e74b74fd"_hex2buf,
                "1000000000000000000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->put("0e348355f6bfaf32d8e947e896fe3536"_hex2buf,
                "1000000000000000000000000000000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(db->put("0deef7973bb5154589f1438f1f21d3a1"_hex2buf,
                                  "00000000"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("1c9f2b74227c8236327fe66693641e42"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("77f9812b2b420bfcce4391ad7ae9451b"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("6301b8a945c24ad6ef0fd5d063c34f19"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("94310e87b78915dd7e2656ce42bda477"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("8cb577756012d928f17362e0741f9f2c"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("3ae31af9a378162eb2736f26855c9ad8"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("f7787e54bb33faaf40a7f3bf438458ee"_hex2buf));
    EXPECT_OUTCOME_TRUE_1(
        db->remove("e1856c2284210a998e3257b3d8a315db"_hex2buf));
    auto root2 = db->getRootHash();

    ASSERT_EQ(root1, root2);
    FAIL() << root.toHex();
  }
  EXPECT_OUTCOME_TRUE(new_level_db,
                      LevelDB::create("/tmp/kagome_leveldb_persistency_test"));
  auto db = PolkadotTrieDb::createFromStorage(
      root,
      std::make_shared<TrieDbBackendImpl>(std::move(new_level_db),
                                          kNodePrefix));
  EXPECT_OUTCOME_TRUE(v1, db->get("123"_buf));
  ASSERT_EQ(v1, "abc"_buf);
  EXPECT_OUTCOME_TRUE(v2, db->get("345"_buf));
  ASSERT_EQ(v2, "def"_buf);
  EXPECT_OUTCOME_TRUE(v3, db->get("678"_buf));
  ASSERT_EQ(v3, "xyz"_buf);

  fs::remove_all("/tmp/kagome_leveldb_persistency_test");
}
