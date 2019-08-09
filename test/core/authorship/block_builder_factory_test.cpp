/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/block_builder_factory_impl.hpp"

#include <gtest/gtest.h>
#include "mock/core/blockchain/header_backend_mock.hpp"
#include "mock/core/runtime/block_builder_api_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "testutil/outcome.hpp"

using ::testing::Return;

using kagome::authorship::BlockBuilderFactoryImpl;
using kagome::blockchain::HeaderRepositoryMock;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::Digest;
using kagome::runtime::BlockBuilderApiMock;
using kagome::runtime::CoreMock;

class BlockBuilderFactoryTest : public ::testing::Test {
 public:
  void SetUp() override {
    expected_hash_.fill(0);
    block_id_ = expected_hash_;

    expected_header_.parent_hash = expected_hash_;
    expected_header_.number = expected_number_;
    expected_header_.digests = {inherent_digest_};

    EXPECT_CALL(*header_backend_, getNumberByHash(expected_hash_))
        .WillOnce(Return(expected_number_));
  }

  std::shared_ptr<CoreMock> core_ = std::make_shared<CoreMock>();
  std::shared_ptr<BlockBuilderApiMock> block_builder_api_ =
      std::make_shared<BlockBuilderApiMock>();
  std::shared_ptr<HeaderRepositoryMock> header_backend_ =
      std::make_shared<HeaderRepositoryMock>();

  BlockNumber expected_number_{42};
  kagome::common::Hash256 expected_hash_;
  BlockId block_id_;
  Digest inherent_digest_{0, 1, 2, 3};
  BlockHeader expected_header_;
};

/**
 * @given HeaderBackend providing expected_hash and expected_number of the
 * block, which become part of expected_block_header
 * @when core runtime successfully initialises expected_block_header
 * @then BlockBuilderFactory that uses this core runtime and HeaderBackend
 * successfully creates BlockBuilder
 */
TEST_F(BlockBuilderFactoryTest, CreateSuccessful) {
  // given
  EXPECT_CALL(*core_, initialise_block(expected_header_))
      .WillOnce(Return(outcome::success()));
  BlockBuilderFactoryImpl factory(core_, block_builder_api_, header_backend_);

  // when
  auto block_builder_res = factory.create(block_id_, inherent_digest_);

  // then
  ASSERT_TRUE(block_builder_res);
}

/**
 * @given HeaderBackend providing expected_hash and expected_number of the
 * block, which become part of expected_block_header
 * @when core runtime does not initialise expected_block_header
 * @then BlockBuilderFactory that uses this core runtime and HeaderBackend
 * does not create BlockBuilder
 */
TEST_F(BlockBuilderFactoryTest, CreateFailed) {
  // given
  EXPECT_CALL(*core_, initialise_block(expected_header_))
      .WillOnce(Return(outcome::failure(boost::system::error_code{})));
  BlockBuilderFactoryImpl factory(core_, block_builder_api_, header_backend_);

  // when
  auto block_builder_res = factory.create(block_id_, inherent_digest_);

  // then
  ASSERT_FALSE(block_builder_res);
}
