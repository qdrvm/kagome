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
using kagome::primitives::BlockNumber;
using testing::ContainerEq;

TEST(JustificationStoragePolicyTest, ShouldStore512Multiples) {
  JustificationStoragePolicyImpl policy{};

  ASSERT_THAT(
      policy.shouldStoreWhatWhenFinalized({0, "genesis"_hash256}).value(),
      ContainerEq(std::vector<BlockNumber>{0}));
  ASSERT_THAT(policy.shouldStoreWhatWhenFinalized({1, "hash1"_hash256}),
              ContainerEq(std::vector<BlockNumber>{}));
  ASSERT_THAT(policy.shouldStoreWhatWhenFinalized({2, "hash2"_hash256}),
              ContainerEq(std::vector<BlockNumber>{}));
  ASSERT_THAT(policy.shouldStoreWhatWhenFinalized({512, "hash512"_hash256}),
              ContainerEq(std::vector<BlockNumber>{512}));
  ASSERT_THAT(policy.shouldStoreWhatWhenFinalized({1024, "hash1024"_hash256}),
              ContainerEq(std::vector<BlockNumber>{1024}));
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
