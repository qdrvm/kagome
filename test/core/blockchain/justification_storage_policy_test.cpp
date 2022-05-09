/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "mock/core/blockchain/block_tree_mock.hpp"

#include "blockchain/impl/justification_storage_policy.hpp"
#include "mock/core/consensus/authority/authority_manager_mock.hpp"
#include "testutil/literals.hpp"

using testing::Return;

using kagome::authority::AuthorityManagerMock;
using kagome::blockchain::JustificationStoragePolicyImpl;
using kagome::common::Hash256;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::primitives::BlockNumber;

BlockHeader makeBlockHeader(const BlockNumber &number) {
  BlockHeader header;
  header.number = number;
  if (number == 0) {
    header.parent_hash = {};
  } else if (number == 1) {
    header.parent_hash = "genesis_hash"_hash256;
  } else {
    auto hash_str = "hash_" + std::to_string(number);
    std::fill_n(hash_str.begin(), hash_str.size(), 0);
    std::copy_n(hash_str.begin(), hash_str.size(), header.parent_hash.begin());
  }
  return header;
}

TEST(JustificationStoragePolicyTest, ShouldStore512Multiples) {
  JustificationStoragePolicyImpl policy{};
  auto tree = std::make_shared<kagome::blockchain::BlockTreeMock>();
  policy.initBlockchainInfo(tree);

  EXPECT_CALL(*tree, getLastFinalized())
      .WillRepeatedly(Return(BlockInfo{"finalized"_hash256, 2000}));

  ASSERT_EQ(policy.shouldStoreFor(makeBlockHeader(0)).value(), true);

  ASSERT_EQ(policy.shouldStoreFor(makeBlockHeader(1)).value(), false);

  ASSERT_EQ(policy.shouldStoreFor(makeBlockHeader(2)).value(), false);

  ASSERT_EQ(policy.shouldStoreFor(makeBlockHeader(512)).value(), true);

  ASSERT_EQ(policy.shouldStoreFor(makeBlockHeader(1024)).value(), true);
}

/**
 * GIVEN finalized block 34
 * WHEN finalizing block 36, which changes authority set
 * THEN justifications of blocks 34 and 36 must be stored
 */
TEST(JustificationStoragePolicyTest, ShouldStoreOnAuthorityChange) {
  JustificationStoragePolicyImpl policy{};
  auto tree = std::make_shared<kagome::blockchain::BlockTreeMock>();
  policy.initBlockchainInfo(tree);

  EXPECT_CALL(*tree, getLastFinalized())
      .WillRepeatedly(Return(BlockInfo{"finalized"_hash256, 2000}));

  {
    auto header = makeBlockHeader(13);
    header.digest.emplace_back(
        kagome::primitives::Consensus{kagome::primitives::ScheduledChange{}});
    ASSERT_EQ(policy.shouldStoreFor(header).value(), true);
  }
  {
    auto header = makeBlockHeader(13);
    header.digest.emplace_back(
        kagome::primitives::Consensus{kagome::primitives::ForcedChange{}});
    ASSERT_EQ(policy.shouldStoreFor(header).value(), true);
  }
  {
    auto header = makeBlockHeader(13);
    header.digest.emplace_back(
        kagome::primitives::Consensus{kagome::primitives::OnDisabled{}});
    ASSERT_EQ(policy.shouldStoreFor(header).value(), false);
  }

}
