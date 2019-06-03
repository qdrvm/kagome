/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/merkle/polkadot_trie_db/polkadot_trie_db.hpp"

#include <gtest/gtest.h>
#include "crypto/hasher/hasher_impl.hpp"
#include "scale/buffer_codec.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/merkle/polkadot_trie_db/polkadot_trie_db_printer.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_leveldb_test.hpp"

using kagome::common::Buffer;
using kagome::hash::HasherImpl;
using kagome::scale::BufferScaleCodec;
using kagome::storage::merkle::PolkadotTrieDb;

class TrieTest : public testing::Test {
 public:
  std::shared_ptr<BufferScaleCodec> codec =
      std::make_shared<BufferScaleCodec>();

  std::vector<std::pair<Buffer, Buffer>> data = {
      {"123456"_hex2buf, "42"_hex2buf},
      {"1234"_hex2buf, "1234"_hex2buf},
      {"010203"_hex2buf, "0a0b"_hex2buf},
      {"010a0b"_hex2buf, "1337"_hex2buf},
      {"0a0b0c"_hex2buf, "deadbeef"_hex2buf}};
};

class MapDb : public kagome::storage::face::PersistedMap<Buffer, Buffer> {
 public:
  outcome::result<Buffer> get(const Buffer &key) const override {
    try {
      return storage.at(key.toHex());
    } catch (std::exception&) {
      std::cout << "error on get " << key.toHex() << "\n";
    }
    return outcome::success();
  }

  outcome::result<void> put(const Buffer &key, const Buffer &value) override {
    try {
      storage[key.toHex()] = value;
    } catch (std::exception&) {
      std::cout << "error on put " << key.toHex() << "\n";
    }
    return outcome::success();
  }

  bool contains(const Buffer &key) const override {
    return storage.find(key.toHex()) != storage.end();
  }

  outcome::result<void> remove(const Buffer &key) override {
    storage.erase(key.toHex());
    return outcome::success();
  }

  std::unique_ptr<kagome::storage::face::WriteBatch<Buffer, Buffer>> batch()
      override {
    return nullptr;
  }

  std::unique_ptr<kagome::storage::face::MapCursor<Buffer, Buffer>> cursor()
      override {
    return nullptr;
  }

  std::map<std::string, Buffer> storage;
};

TEST_F(TrieTest, Create) {
  PolkadotTrieDb trie(std::make_unique<MapDb>(), codec,
                      std::make_shared<HasherImpl>());
}

TEST_F(TrieTest, Put) {
  PolkadotTrieDb trie(std::make_unique<MapDb>(), codec,
                      std::make_shared<HasherImpl>());

  for (auto &entry : data) {
    EXPECT_OUTCOME_TRUE_void(_, trie.put(entry.first, entry.second));
  }

  for (auto &entry : data) {
    EXPECT_OUTCOME_TRUE(res, trie.get(entry.first));
    ASSERT_EQ(res, entry.second);
  }
}

TEST_F(TrieTest, Remove) {
  PolkadotTrieDb trie(std::make_unique<MapDb>(), codec,
                      std::make_shared<HasherImpl>());

  for (auto &entry : data) {
    EXPECT_OUTCOME_TRUE_void(_, trie.put(entry.first, entry.second));
  }
  EXPECT_OUTCOME_TRUE_void(_, trie.remove(data[2].first));
  ASSERT_FALSE(trie.contains(data[2].first));
}

TEST_F(TrieTest, Replace) {
  PolkadotTrieDb trie(std::make_unique<MapDb>(), codec,
                      std::make_shared<HasherImpl>());

  for (auto &entry : data) {
    EXPECT_OUTCOME_TRUE_void(_, trie.put(entry.first, entry.second));
  }
  EXPECT_OUTCOME_TRUE_void(__, trie.put(data[1].first, data[3].second));
  EXPECT_OUTCOME_TRUE(res, trie.get(data[1].first));
  ASSERT_EQ(res, data[3].second);
}
