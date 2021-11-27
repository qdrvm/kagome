/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "primitives/event_types.hpp"
#include "scale/scale.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/persistent_trie_batch_impl.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::api::Session;
using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::primitives::BlockHash;
using kagome::primitives::ExtrinsicIndex;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::primitives::events::StorageSubscriptionEngine;
using kagome::storage::InMemoryStorage;
using kagome::storage::changes_trie::ChangesTracker;
using kagome::storage::changes_trie::StorageChangesTrackerImpl;
using kagome::storage::trie::PersistentTrieBatch;
using kagome::storage::trie::PersistentTrieBatchImpl;
using kagome::storage::trie::PolkadotCodec;
using kagome::storage::trie::PolkadotTrieFactoryImpl;
using kagome::storage::trie::TrieSerializerImpl;
using kagome::storage::trie::TrieStorageBackendImpl;
using testing::_;
using testing::AnyOf;
using testing::Return;

/**
 * @given storage batch with pending changes
 * @when initialize a changes trie with the changes
 * @then changes are passed to the trie successfully
 */
TEST(ChangesTrieTest, IntegrationWithOverlay) {
  testutil::prepareLoggers();

  // GIVEN
  auto factory = std::make_shared<PolkadotTrieFactoryImpl>();
  auto codec = std::make_shared<PolkadotCodec>();
  auto backend = std::make_shared<TrieStorageBackendImpl>(
      std::make_shared<InMemoryStorage>(), Buffer{});
  auto serializer =
      std::make_shared<TrieSerializerImpl>(factory, codec, backend);
  auto storage_subscription_engine =
      std::make_shared<StorageSubscriptionEngine>();
  auto chain_subscription_engine = std::make_shared<ChainSubscriptionEngine>();
  std::shared_ptr<ChangesTracker> changes_tracker =
      std::make_shared<StorageChangesTrackerImpl>(factory,
                                                  codec,
                                                  storage_subscription_engine,
                                                  chain_subscription_engine);
  EXPECT_OUTCOME_TRUE_1(
      changes_tracker->onBlockExecutionStart("aaa"_hash256, 42));
  auto batch = PersistentTrieBatchImpl::create(
      codec,
      serializer,
      std::make_optional(changes_tracker),
      factory->createEmpty([](auto &) { return outcome::success(); }));

  EXPECT_OUTCOME_TRUE_1(
      batch->put(":extrinsic_index"_buf, Buffer{scale::encode(42).value()}));
  EXPECT_OUTCOME_TRUE_1(batch->put("abc"_buf, "123"_buf));
  EXPECT_OUTCOME_TRUE_1(batch->put("cde"_buf, "345"_buf));

  auto repo = std::make_shared<BlockHeaderRepositoryMock>();
  EXPECT_CALL(*repo, getNumberByHash(_)).WillRepeatedly(Return(42));

  EXPECT_OUTCOME_TRUE_1(
      changes_tracker->constructChangesTrie("aaa"_hash256, {}));
  // THEN SUCCESS
}
