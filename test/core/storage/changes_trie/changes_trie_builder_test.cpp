/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "storage/changes_trie/impl/changes_trie_builder_impl.hpp"

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "scale/scale.hpp"
#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/polkadot_trie_factory_impl.hpp"
#include "storage/trie/impl/trie_serializer_impl.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::storage::InMemoryStorage;
using kagome::storage::changes_trie::ChangesTrieBuilderImpl;
using kagome::storage::trie::PolkadotCodec;
using kagome::storage::trie::PolkadotTrieFactoryImpl;
using kagome::storage::trie::TrieStorageMock;
using testing::_;
using testing::Return;

/**
 * @given a changes trie with congifuration identical to a one in a substrate
 * test
 * @when calculationg its hash
 * @then it matches the hash from substrate
 */
TEST(ChangesTrieTest, SubstrateCompatibility) {
  auto factory = std::make_shared<PolkadotTrieFactoryImpl>();
  auto codec = std::make_shared<PolkadotCodec>();

  auto storage_mock = std::make_shared<TrieStorageMock>();
  auto repo = std::make_shared<BlockHeaderRepositoryMock>();
  EXPECT_CALL(*repo, getNumberByHash(_)).WillRepeatedly(Return(99));
  ChangesTrieBuilderImpl changes_trie_builder(
      storage_mock, factory, repo, codec);

  std::vector<
      std::pair<Buffer, std::vector<kagome::primitives::ExtrinsicIndex>>>
      changes{{Buffer{1}, {1}}, {":extrinsic_index"_buf, {1}}};

  EXPECT_OUTCOME_FALSE(err, changes_trie_builder.insertExtrinsicsChange(
      changes.front().first, changes.front().second));
  ASSERT_EQ(err, ChangesTrieBuilderImpl::Error::TRIE_NOT_INITIALIZED);
  EXPECT_OUTCOME_TRUE_1(changes_trie_builder.startNewTrie("aaa"_hash256));
  for (auto &change : changes) {
    auto &key = change.first;
    EXPECT_OUTCOME_TRUE_1(
        changes_trie_builder.insertExtrinsicsChange(key, change.second));
  }
  auto hash = changes_trie_builder.finishAndGetHash();
  ASSERT_EQ(
      hash,
      kagome::common::Hash256::fromHex(
          "bb0c2ef6e1d36d5490f9766cfcc7dfe2a6ca804504c3bb206053890d6dd02376")
          .value());
}
