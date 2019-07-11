/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "basic_authorship/impl/block_builder_factory_impl.hpp"
#include "mock/core/blockchain/header_backend_mock.hpp"
#include "mock/core/runtime/block_builder_api_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "testutil/outcome.hpp"

using ::testing::Return;

using kagome::basic_authorship::BlockBuilderFactoryImpl;
using kagome::blockchain::HeaderBackendMock;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::Digest;
using kagome::runtime::BlockBuilderApiMock;
using kagome::runtime::CoreMock;

TEST(BlockBuilderFactoryTest, CreateTest) {
  auto core = std::make_shared<CoreMock>();
  auto block_builder_api = std::make_shared<BlockBuilderApiMock>();
  auto header_backend = std::make_shared<HeaderBackendMock>();

  BlockNumber expected_number{42};

  kagome::common::Hash256 expected_hash;
  expected_hash.fill(0);
  BlockId block_id = expected_hash;

  Digest inherent_digest{0, 1, 2, 3};

  BlockHeader expected_header;
  expected_header.parent_hash = expected_hash;
  expected_header.number = expected_number;
  expected_header.digest = inherent_digest;

  BlockBuilderFactoryImpl factory(core, block_builder_api, header_backend);
  EXPECT_CALL(*header_backend, hashFromBlockId(block_id))
      .WillOnce(Return(expected_hash));
  EXPECT_CALL(*header_backend, numberFromBlockId(block_id))
      .WillOnce(Return(expected_number));
  EXPECT_CALL(*core, initialise_block(expected_header))
      .WillOnce(Return(outcome::success()));

  auto block_builder_res = factory.create(block_id, inherent_digest);
  EXPECT_OUTCOME_TRUE(block_builder, block_builder_res);
}
