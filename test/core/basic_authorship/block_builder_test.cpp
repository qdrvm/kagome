/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "basic_authorship/impl/block_builder_impl.hpp"

#include <gtest/gtest.h>
#include "mock/core/runtime/block_builder_api_mock.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Return;

using kagome::basic_authorship::BlockBuilderImpl;
using kagome::primitives::Block;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockNumber;
using kagome::primitives::Extrinsic;
using kagome::primitives::InherentData;
using kagome::runtime::BlockBuilderApiMock;

class BlockBuilderTest : public ::testing::Test {
 public:
  void SetUp() override {
    // add some number to the header to make it possible to differentiate it
    expected_header_.number = number_;

    block_builder_ = std::make_shared<BlockBuilderImpl>(expected_header_,
                                                        block_builder_api_);
  }

 protected:
  std::shared_ptr<BlockBuilderApiMock> block_builder_api_ =
      std::make_shared<BlockBuilderApiMock>();

  BlockHeader expected_header_;
  BlockNumber number_ = 123;

  std::shared_ptr<BlockBuilderImpl> block_builder_;
};

/**
 * @given BlockBuilderApi that fails to apply extrinsic @and BlockBuilder that
 * uses that BlockBuilderApi
 * @when BlockBuilder tries to push extrinsic @and BlockBuilder bakes a block
 * @then push fails
 */
TEST_F(BlockBuilderTest, PushWhenApplyFails) {
  // given
  Extrinsic xt{};
  EXPECT_CALL(*block_builder_api_, apply_extrinsic(xt))
      .WillOnce(Return(outcome::failure(boost::system::error_code{})));

  // when
  auto res = block_builder_->pushExtrinsic(xt);
  auto block = block_builder_->bake();

  // then
  ASSERT_FALSE(res);
  ASSERT_THAT(block.extrinsics, IsEmpty());
}

/**
 * @given BlockBuilderApi that returns false to apply extrinsic @and
 * BlockBuilder that uses that BlockBuilderApi
 * @when BlockBuilder tries to push extrinsic @and BlockBuilder bakes a block
 * @then Extrinsic is not added to the baked block
 */
TEST_F(BlockBuilderTest, PushWhenApplySucceedsWithTrue) {
  // given
  Extrinsic xt{};
  EXPECT_CALL(*block_builder_api_, apply_extrinsic(xt)).WillOnce(Return(true));

  // when
  auto res = block_builder_->pushExtrinsic(xt);
  ASSERT_TRUE(res);

  auto block = block_builder_->bake();

  // then
  ASSERT_EQ(block.header, expected_header_);
  ASSERT_THAT(block.extrinsics, ElementsAre(xt));
}

/**
 * @given BlockBuilderApi that returns true to apply extrinsic @and
 * BlockBuilder that uses that BlockBuilderApi
 * @when BlockBuilder tries to push extrinsic @and BlockBuilder bakes a block
 * @then Extrinsic is added to the baked block
 */
TEST_F(BlockBuilderTest, PushWhenApplySucceedsWithFalse) {
  // given
  Extrinsic xt{};
  EXPECT_CALL(*block_builder_api_, apply_extrinsic(xt)).WillOnce(Return(false));

  // when
  auto res = block_builder_->pushExtrinsic(xt);
  ASSERT_TRUE(res);

  // then
  auto block = block_builder_->bake();
  ASSERT_EQ(block.header, expected_header_);
  ASSERT_THAT(block.extrinsics, IsEmpty());
}
