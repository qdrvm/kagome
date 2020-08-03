/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/trie_storage_impl.hpp"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "outcome/outcome.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "subscription/subscriber.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::api::Session;
using kagome::common::Buffer;
using kagome::primitives::BlockHash;
using kagome::storage::LevelDB;
using kagome::storage::trie::PolkadotCodec;
using kagome::storage::trie::PolkadotTrieFactoryImpl;
using kagome::storage::trie::TrieSerializerImpl;
using kagome::storage::trie::TrieStorageBackendImpl;
using kagome::storage::trie::TrieStorageImpl;
using kagome::subscription::SubscriptionEngine;

static Buffer kNodePrefix = "\1"_buf;

/**
 * @given an empty persistent trie with LevelDb backend
 * @when putting a value into it @and its intance is destroyed @and a new
 * instance initialsed with the same DB
 * @then the new instance contains the same data
 */
TEST(TriePersistencyTest, CreateDestroyCreate) {
  Buffer root;
  auto factory = std::make_shared<PolkadotTrieFactoryImpl>();
  auto codec = std::make_shared<PolkadotCodec>();
  {
    leveldb::Options options;
    options.create_if_missing = true;  // intentionally
    EXPECT_OUTCOME_TRUE(
        level_db,
        LevelDB::create("/tmp/kagome_leveldb_persistency_test", options));
    auto serializer = std::make_shared<TrieSerializerImpl>(
        factory,
        codec,
        std::make_shared<TrieStorageBackendImpl>(std::move(level_db),
                                                 kNodePrefix));

    auto storage =
        TrieStorageImpl::createEmpty(factory, codec, serializer, boost::none)
            .value();

    auto batch = storage->getPersistentBatch().value();
    EXPECT_OUTCOME_TRUE_1(batch->put("123"_buf, "abc"_buf));
    EXPECT_OUTCOME_TRUE_1(batch->put("345"_buf, "def"_buf));
    EXPECT_OUTCOME_TRUE_1(batch->put("678"_buf, "xyz"_buf));
    EXPECT_OUTCOME_TRUE(root_, batch->commit());
    root = root_;
  }
  EXPECT_OUTCOME_TRUE(new_level_db,
                      LevelDB::create("/tmp/kagome_leveldb_persistency_test"));
  auto serializer = std::make_shared<TrieSerializerImpl>(
      factory,
      codec,
      std::make_shared<TrieStorageBackendImpl>(std::move(new_level_db),
                                               kNodePrefix));
  auto storage =
      TrieStorageImpl::createFromStorage(root, codec, serializer, boost::none)
          .value();
  auto batch = storage->getPersistentBatch().value();
  EXPECT_OUTCOME_TRUE(v1, batch->get("123"_buf));
  ASSERT_EQ(v1, "abc"_buf);
  EXPECT_OUTCOME_TRUE(v2, batch->get("345"_buf));
  ASSERT_EQ(v2, "def"_buf);
  EXPECT_OUTCOME_TRUE(v3, batch->get("678"_buf));
  ASSERT_EQ(v3, "xyz"_buf);

  boost::filesystem::remove_all("/tmp/kagome_leveldb_persistency_test");
}
