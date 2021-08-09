/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/block_builder_impl.hpp"

#include <gtest/gtest.h>
#include "mock/core/runtime/block_builder_api_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Return;

using kagome::authorship::BlockBuilderImpl;
using kagome::primitives::Block;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::primitives::BlockNumber;
using kagome::primitives::DispatchError;
using kagome::primitives::DispatchOutcome;
using kagome::primitives::DispatchSuccess;
using kagome::primitives::Extrinsic;
using kagome::primitives::InherentData;
using kagome::primitives::dispatch_error::Other;
using kagome::runtime::BlockBuilderApiMock;
using kagome::runtime::PersistentResult;
using kagome::storage::trie::RootHash;

class BlockBuilderTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    // add some number to the header to make it possible to differentiate it
    expected_header_.number = block_number_;
    expected_header_.state_root = "state123"_hash256;
    expected_header_.parent_hash = "block122"_hash256;

    parent_block_ = BlockInfo{block_number_ - 1, expected_header_.parent_hash};

    block_builder_ = std::make_shared<BlockBuilderImpl>(
        expected_header_, initial_state_, block_builder_api_);
  }

 protected:
  std::shared_ptr<BlockBuilderApiMock> block_builder_api_ =
      std::make_shared<BlockBuilderApiMock>();

  BlockHeader expected_header_;
  BlockNumber block_number_ = 123;
  RootHash initial_state_ = "init_state"_hash256;
  BlockInfo parent_block_;

  std::shared_ptr<BlockBuilderImpl> block_builder_;
};

/**
 * @given BlockBuilderApi that fails to apply extrinsic @and BlockBuilder that
 * uses that BlockBuilderApi
 * @when BlockBuilder tries to push extrinsic @and BlockBuilder bakes a block
 * @then push fails @and created block is empty
 */
TEST_F(BlockBuilderTest, PushWhenApplyFails) {
  // given
  Extrinsic xt{};
  EXPECT_CALL(*block_builder_api_,
              apply_extrinsic(parent_block_, initial_state_, xt))
      .WillOnce(Return(outcome::failure(boost::system::error_code{})));
  EXPECT_CALL(*block_builder_api_,
              finalize_block(parent_block_, initial_state_))
      .WillOnce(Return(expected_header_));

  // when
  auto res = block_builder_->pushExtrinsic(xt);
  EXPECT_OUTCOME_TRUE(block, block_builder_->bake());

  // then
  ASSERT_FALSE(res);
  ASSERT_THAT(block.body, IsEmpty());
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
  EXPECT_CALL(*block_builder_api_,
              apply_extrinsic(parent_block_, initial_state_, xt))
      .WillOnce(Return(outcome::success(
          PersistentResult<kagome::primitives::ApplyExtrinsicResult>{
              DispatchSuccess{}, "next_state"_hash256})));
  EXPECT_CALL(*block_builder_api_,
              finalize_block(parent_block_, "next_state"_hash256))
      .WillOnce(Return(expected_header_));

  // when
  auto res = block_builder_->pushExtrinsic(xt);
  ASSERT_TRUE(res);

  EXPECT_OUTCOME_TRUE(block, block_builder_->bake());

  // then
  ASSERT_EQ(block.header, expected_header_);
  ASSERT_THAT(block.body, ElementsAre(xt));
}

/**
 * @given BlockBuilderApi that returns true to apply extrinsic @and
 * BlockBuilder that uses that BlockBuilderApi
 * @when BlockBuilder tries to push extrinsic @and BlockBuilder bakes a block
 * @then Extrinsic is not added to the baked block
 */
TEST_F(BlockBuilderTest, PushWhenApplySucceedsWithFalse) {
  // given
  Extrinsic xt{};
  EXPECT_CALL(*block_builder_api_,
              apply_extrinsic(parent_block_, initial_state_, xt))
      .WillOnce(Return(kagome::runtime::PersistentResult<
                       kagome::primitives::ApplyExtrinsicResult>{
          DispatchError{Other{}}, "next_state"_hash256}));
  EXPECT_CALL(*block_builder_api_,
              finalize_block(parent_block_, "next_state"_hash256))
      .WillOnce(Return(expected_header_));

  // when
  auto res = block_builder_->pushExtrinsic(xt);

  // then
  ASSERT_TRUE(res);
  EXPECT_OUTCOME_TRUE(block, block_builder_->bake());
  ASSERT_EQ(block.header, expected_header_);
  ASSERT_THAT(block.body, testing::Not(IsEmpty()));
}
