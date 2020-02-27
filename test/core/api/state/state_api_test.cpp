/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "api/state/impl/state_api_impl.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/blockchain/header_repository_mock.hpp"
#include "mock/core/storage/trie/trie_db_mock.hpp"
#include "primitives/block_header.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::api::ReadonlyTrieBuilder;
using kagome::blockchain::BlockTreeMock;
using kagome::blockchain::HeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::storage::trie::TrieDbMock;
using kagome::storage::trie::TrieDbReader;
using testing::Return;

/**
 * Builds mock trees.
 * Because buildAt must return a unique_ptr, be careful,
 * next_trie is updated to a new instance after every buildAt call
 */
class TrieMockBuilder : public ReadonlyTrieBuilder {
 public:
  TrieMockBuilder() {
    next_trie = std::make_unique<TrieDbMock>();
  }
  ~TrieMockBuilder() override = default;

  std::unique_ptr<TrieDbReader> buildAt(BlockHash state_root) const override {
    std::unique_ptr<TrieDbReader> trie{std::move(next_trie)};
    next_trie = std::make_unique<TrieDbMock>();
    return trie;
  }

  mutable std::unique_ptr<TrieDbMock> next_trie{};
};

/**
 * @given state api
 * @when get a storage value for the given key (and optionally state root)
 * @then the correct value is returned
 */
TEST(StateApiTest, GetStorage) {
  auto builder = std::make_shared<TrieMockBuilder>();
  auto block_header_repo = std::make_shared<HeaderRepositoryMock>();
  auto block_tree = std::make_shared<BlockTreeMock>();

  kagome::api::StateApiImpl api{block_header_repo, builder, block_tree};

  EXPECT_CALL(*block_tree, getLastFinalized())
      .WillOnce(testing::Return(BlockInfo(42, "D"_hash256)));
  kagome::primitives::BlockId did = "D"_hash256;
  EXPECT_CALL(*block_header_repo, getBlockHeader(did))
      .WillOnce(testing::Return(BlockHeader{.state_root = "CDE"_hash256}));
  EXPECT_CALL(*builder->next_trie, get("a"_buf))
      .WillOnce(testing::Return("1"_buf));

  EXPECT_OUTCOME_TRUE(r, api.getStorage("a"_buf));
  ASSERT_EQ(r, "1"_buf);

  kagome::primitives::BlockId bid = "B"_hash256;
  EXPECT_CALL(*block_header_repo, getBlockHeader(bid))
      .WillOnce(testing::Return(BlockHeader{.state_root = "ABC"_hash256}));
  EXPECT_CALL(*builder->next_trie, get("a"_buf))
      .WillOnce(testing::Return("1"_buf));

  EXPECT_OUTCOME_TRUE(r1, api.getStorage("a"_buf, "B"_hash256));
  ASSERT_EQ(r1, "1"_buf);
}
