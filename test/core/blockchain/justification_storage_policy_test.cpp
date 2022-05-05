/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "blockchain/impl/justification_storage_policy.hpp"
#include "mock/core/consensus/authority/authority_manager_mock.hpp"
#include "testutil/literals.hpp"

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
    header.parent_hash =
        Hash256::fromString("hash_" + std::to_string(number)).value();
  }
  return header;
}

TEST(JustificationStoragePolicyTest, ShouldStore512Multiples) {
  JustificationStoragePolicyImpl policy{};

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
  auto authority_manager = std::make_shared<AuthorityManagerMock>();
}
