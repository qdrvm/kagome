/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "storage/changes_trie/impl/changes_trie_builder_impl.hpp"

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::storage::changes_trie::ChangesTrieBuilderImpl;
using kagome::storage::trie::TrieStorage;
using kagome::storage::trie::PersistentTrieBatch;
using kagome::common::Buffer;
using testing::_;
using testing::Return;

/**
 * @given storage overlay with pending changes
 * @when initialize a changes trie with the changes
 * @then changes are passed to the trie successfully
 */
TEST(ChangesTrieTest, IntegrationWithOverlay) {
  // GIVEN
  auto storage = std::make_shared<TrieStorage>();
  auto batch = std::make_shared<PersistentTrieBatch>(factory->makeTrieDb());
  EXPECT_OUTCOME_TRUE_1(batch->put("abc"_buf, "123"_buf));
  EXPECT_OUTCOME_TRUE_1(batch->put("cde"_buf, "345"_buf));

  auto repo = std::make_shared<BlockHeaderRepositoryMock>();
  EXPECT_CALL(*repo, getNumberByHash(_)).WillRepeatedly(Return(42));

  // WHEN
  auto changes_trie_builder = ChangesTrieBuilderImpl(overlay, factory, repo);
  EXPECT_OUTCOME_TRUE_1(overlay->sinkChangesTo(changes_trie_builder));
  // THEN SUCCESS
}

/**
 * @given a changes trie with congifuration identical to a one in a substrate
 * test
 * @when calculationg its hash
 * @then it matches the hash from substrate
 */
TEST(ChangesTrieTest, SubstrateCompatibility) {
  auto storage = std::make_shared<TrieStorage>();
  auto batch = std::make_shared<PersistentTrieBatch>(factory->makeTrieDb());
  auto repo = std::make_shared<BlockHeaderRepositoryMock>();
  EXPECT_CALL(*repo, getNumberByHash(_)).WillRepeatedly(Return(99));
  auto changes_trie_builder = ChangesTrieBuilderImpl(overlay, factory, repo);

  std::vector<
      std::pair<Buffer, std::vector<kagome::primitives::ExtrinsicIndex>>>
      changes{{Buffer{1}, {1}}, {":extrinsic_index"_buf, {1}}};
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
