/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "mock/core/blockchain/block_tree_mock.hpp"

#include "blockchain/impl/justification_storage_policy.hpp"
#include "mock/core/consensus/grandpa/authority_manager_mock.hpp"
#include "testutil/lazy.hpp"
#include "testutil/literals.hpp"
#include "primitives/common.hpp"

using testing::Return;

using kagome::blockchain::JustificationStoragePolicyImpl;
using kagome::common::Hash256;
using kagome::consensus::grandpa::AuthorityManagerMock;
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

class JustificationStoragePolicyTest : public testing::Test {
 public:
  static constexpr kagome::primitives::BlockNumber last_finalized_number = 2000;
  void SetUp() override {
    policy_.initBlockchainInfo();
  }

 protected:
  JustificationStoragePolicyImpl policy_{};
};
TEST_F(JustificationStoragePolicyTest, ShouldStore512Multiples) {
  ASSERT_EQ(policy_.shouldStoreFor(makeBlockHeader(0), last_finalized_number).value(), true);
  ASSERT_EQ(policy_.shouldStoreFor(makeBlockHeader(1), last_finalized_number).value(), false);
  ASSERT_EQ(policy_.shouldStoreFor(makeBlockHeader(2), last_finalized_number).value(), false);
  ASSERT_EQ(policy_.shouldStoreFor(makeBlockHeader(512), last_finalized_number).value(), true);
  ASSERT_EQ(policy_.shouldStoreFor(makeBlockHeader(1024), last_finalized_number).value(), true);
}

/**
 * GIVEN finalized block 13, which contains a ScheduledChange
 * WHEN finalizing block 13
 * THEN justifications of block 13 must be stored
 */
TEST_F(JustificationStoragePolicyTest, ShouldStoreOnScheduledChange) {
  auto header = makeBlockHeader(13);
  header.digest.emplace_back(
      kagome::primitives::Consensus{kagome::primitives::ScheduledChange{}});
  ASSERT_EQ(policy_.shouldStoreFor(header, last_finalized_number).value(), true);
}

/**
 * GIVEN finalized block 13, which contains a ForcedChange
 * WHEN finalizing block 13
 * THEN justifications of block 13 must be stored
 */
TEST_F(JustificationStoragePolicyTest, ShouldStoreOnForcedChange) {
  auto header = makeBlockHeader(13);
  header.digest.emplace_back(
      kagome::primitives::Consensus{kagome::primitives::ForcedChange{}});
  ASSERT_EQ(policy_.shouldStoreFor(header, last_finalized_number).value(), true);
}

/**
 * GIVEN finalized block 13, which contains a Disabled authority set event
 * WHEN finalizing block 13
 * THEN justifications of block 13 must not be stored
 */
TEST_F(JustificationStoragePolicyTest, ShouldStoreOnDisabledChange) {
  auto header = makeBlockHeader(13);
  header.digest.emplace_back(
      kagome::primitives::Consensus{kagome::primitives::OnDisabled{}});
  ASSERT_EQ(policy_.shouldStoreFor(header, last_finalized_number).value(), false);
}
