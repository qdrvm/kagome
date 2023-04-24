/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/trie_storage_impl.hpp"

#include <gtest/gtest.h>

#include "filesystem/common.hpp"
#include "outcome/outcome.hpp"
#include "storage/rocksdb/rocksdb.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "subscription/subscriber.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Buffer;
using kagome::primitives::BlockHash;
using kagome::storage::RocksDb;
using kagome::storage::Space;
using kagome::storage::trie::PolkadotCodec;
using kagome::storage::trie::PolkadotTrieFactoryImpl;
using kagome::storage::trie::RootHash;
using kagome::storage::trie::StateVersion;
using kagome::storage::trie::TrieSerializerImpl;
using kagome::storage::trie::TrieStorageBackendImpl;
using kagome::storage::trie::TrieStorageImpl;
using kagome::subscription::SubscriptionEngine;

/**
 * @given an empty persistent trie with RocksDb backend
 * @when putting a value into it @and its intance is destroyed @and a new
 * instance initialsed with the same DB
 * @then the new instance contains the same data
 */
TEST(TriePersistencyTest, CreateDestroyCreate) {
  testutil::prepareLoggers();

  RootHash root;
  auto factory = std::make_shared<PolkadotTrieFactoryImpl>();
  auto codec = std::make_shared<PolkadotCodec>();
  {
    rocksdb::Options options;
    options.create_if_missing = true;  // intentionally
    EXPECT_OUTCOME_TRUE(
        rocks_db,
        RocksDb::create("/tmp/kagome_rocksdb_persistency_test", options));

    auto serializer = std::make_shared<TrieSerializerImpl>(
        factory,
        codec,
        std::make_shared<TrieStorageBackendImpl>(
            rocks_db->getSpace(Space::kDefault)));

    auto storage =
        TrieStorageImpl::createEmpty(factory, codec, serializer).value();

    auto batch =
        storage
            ->getPersistentBatchAt(serializer->getEmptyRootHash(), std::nullopt)
            .value();
    EXPECT_OUTCOME_TRUE_1(batch->put("123"_buf, "abc"_buf));
    EXPECT_OUTCOME_TRUE_1(batch->put("345"_buf, "def"_buf));
    EXPECT_OUTCOME_TRUE_1(batch->put("678"_buf, "xyz"_buf));
    EXPECT_OUTCOME_TRUE(root_, batch->commit(StateVersion::V0));
    root = root_;
  }
  EXPECT_OUTCOME_TRUE(new_rocks_db,
                      RocksDb::create("/tmp/kagome_rocksdb_persistency_test"));
  auto serializer = std::make_shared<TrieSerializerImpl>(
      factory,
      codec,
      std::make_shared<TrieStorageBackendImpl>(
          new_rocks_db->getSpace(Space::kDefault)));
  auto storage = TrieStorageImpl::createFromStorage(codec, serializer).value();
  auto batch = storage->getPersistentBatchAt(root, std::nullopt).value();
  EXPECT_OUTCOME_TRUE(v1, batch->get("123"_buf));
  ASSERT_EQ(v1, "abc"_buf);
  EXPECT_OUTCOME_TRUE(v2, batch->get("345"_buf));
  ASSERT_EQ(v2, "def"_buf);
  EXPECT_OUTCOME_TRUE(v3, batch->get("678"_buf));
  ASSERT_EQ(v3, "xyz"_buf);

  kagome::filesystem::remove_all("/tmp/kagome_rocksdb_persistency_test");
}
