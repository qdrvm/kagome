/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "blockchain/impl/changes_trie_builder_impl.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "scale/scale.hpp"
#include "storage/trie/impl/in_memory_trie_db_factory.hpp"
#include "storage/trie_db_overlay/impl/trie_db_overlay_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::blockchain::ChangesTrieBuilderImpl;
using kagome::common::Buffer;
using kagome::storage::trie::InMemoryTrieDbFactory;
using kagome::storage::trie_db_overlay::TrieDbOverlayImpl;
using testing::_;
using testing::Return;

TEST(ChangesTrieTest, IntegrationWithOverlay) {
  auto factory = std::make_shared<InMemoryTrieDbFactory>();
  TrieDbOverlayImpl overlay(factory->makeTrieDb());
  EXPECT_OUTCOME_TRUE_1(overlay.put("abc"_buf, "123"_buf));
  EXPECT_OUTCOME_TRUE_1(overlay.put("cde"_buf, "345"_buf));

  auto repo = std::make_shared<BlockHeaderRepositoryMock>();
  EXPECT_CALL(*repo, getNumberByHash(_)).WillRepeatedly(Return(42));

  auto changes_trie_builder = ChangesTrieBuilderImpl({}, {}, factory, repo);
  EXPECT_OUTCOME_TRUE_1(overlay.sinkChangesTo(changes_trie_builder));
  auto hash = changes_trie_builder.finishAndGetHash();
  ASSERT_EQ(
      hash,
      kagome::common::Hash256::fromHex(
          "44cb1f5b9a98567dad54fca3a11444aa6f4d191c5dba8f2efba98dd9944da67f")
          .value());
}

TEST(ChangesTrieTest, TrieCompat) {
    auto factory = std::make_shared<InMemoryTrieDbFactory>();
    auto trie = factory->makeTrieDb();
    EXPECT_EQ(trie->getRootHash().toHex(), "a");
    trie->put(Buffer{1}, Buffer{2});
    EXPECT_EQ(trie->getRootHash().toHex(), "a");
    trie->put(Buffer{5, 7}, Buffer{8, 6});
    EXPECT_EQ(trie->getRootHash().toHex(), "a");
}

TEST(ChangesTrieTest, SubstrateCompatibility) {
  auto factory = std::make_shared<InMemoryTrieDbFactory>();
  auto repo = std::make_shared<BlockHeaderRepositoryMock>();
  EXPECT_CALL(*repo, getNumberByHash(_)).WillRepeatedly(Return(99));
  auto changes_trie_builder = ChangesTrieBuilderImpl({}, {}, factory, repo);

  std::vector<
      std::pair<Buffer, std::vector<kagome::primitives::ExtrinsicIndex>>>
      changes {
          {Buffer{1}, {1}},
          {":extrinsic_index"_buf, {1}}
  };
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
