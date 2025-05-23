/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/trie_storage_impl.hpp"

#include <gtest/gtest.h>

#include <qtils/test/outcome.hpp>

#include "filesystem/common.hpp"
#include "mock/core/storage/trie_pruner/trie_pruner_mock.hpp"
#include "storage/rocksdb/rocksdb.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "subscription/subscriber.hpp"
#include "testutil/literals.hpp"
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
using kagome::storage::trie_pruner::TriePrunerMock;
using kagome::subscription::SubscriptionEngine;
using testing::_;
using testing::Return;

/**
 * @given an empty persistent trie with RocksDb backend
 * @when putting a value into it @and its instance is destroyed @and a new
 * instance initialised with the same DB
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
    ASSERT_OUTCOME_SUCCESS(
        rocks_db,
        RocksDb::create("/tmp/kagome_rocksdb_persistency_test", options));

    auto serializer = std::make_shared<TrieSerializerImpl>(
        factory, codec, std::make_shared<TrieStorageBackendImpl>(rocks_db));

    auto state_pruner = std::make_shared<TriePrunerMock>();
    ON_CALL(*state_pruner,
            addNewState(
                testing::A<const kagome::storage::trie::PolkadotTrie &>(), _))
        .WillByDefault(Return(outcome::success()));

    auto storage =
        TrieStorageImpl::createEmpty(factory, codec, serializer, state_pruner)
            .value();

    auto batch =
        storage
            ->getPersistentBatchAt(serializer->getEmptyRootHash(), std::nullopt)
            .value();
    EXPECT_OUTCOME_SUCCESS(batch->put("123"_buf, "abc"_buf));
    EXPECT_OUTCOME_SUCCESS(batch->put("345"_buf, "def"_buf));
    EXPECT_OUTCOME_SUCCESS(batch->put("678"_buf, "xyz"_buf));
    ASSERT_OUTCOME_SUCCESS(root_, batch->commit(StateVersion::V0));
    root = root_;
  }
  ASSERT_OUTCOME_SUCCESS(
      new_rocks_db, RocksDb::create("/tmp/kagome_rocksdb_persistency_test"));
  auto serializer = std::make_shared<TrieSerializerImpl>(
      factory, codec, std::make_shared<TrieStorageBackendImpl>(new_rocks_db));
  auto state_pruner = std::make_shared<TriePrunerMock>();
  auto storage =
      TrieStorageImpl::createFromStorage(codec, serializer, state_pruner)
          .value();
  auto batch = storage->getPersistentBatchAt(root, std::nullopt).value();
  ASSERT_OUTCOME_SUCCESS(v1, batch->get("123"_buf));
  ASSERT_EQ(v1, "abc"_buf);
  ASSERT_OUTCOME_SUCCESS(v2, batch->get("345"_buf));
  ASSERT_EQ(v2, "def"_buf);
  ASSERT_OUTCOME_SUCCESS(v3, batch->get("678"_buf));
  ASSERT_EQ(v3, "xyz"_buf);

  kagome::filesystem::remove_all("/tmp/kagome_rocksdb_persistency_test");
}
