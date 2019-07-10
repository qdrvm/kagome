/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "basic_authorship/impl/block_builder_impl.hpp"

#include <gtest/gtest.h>
#include "mock/core/runtime/block_builder_api_mock.hpp"

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
    BlockHeader header_{};
    // add some number to the header to make it possible to differentiate it
    header_.number = number_;

    block_builder_ =
        std::make_shared<BlockBuilderImpl>(header_, block_builder_api_);
  }

 protected:
  std::shared_ptr<BlockBuilderApiMock> block_builder_api_ =
      std::make_shared<BlockBuilderApiMock>();

  BlockNumber number_ = 123;

  std::shared_ptr<BlockBuilderImpl> block_builder_;
};

namespace std {
  std::ostream &operator<<(std::ostream &s, const Extrinsic &xt) {
    return testing::internal2::operator<<(s, xt);
  }

  std::ostream &operator<<(std::ostream &s, const InherentData &inherent_data) {
    return testing::internal2::operator<<(s, inherent_data);
  }

  std::ostream &operator<<(std::ostream &s, const Block &block) {
    return testing::internal2::operator<<(s, block);
  }
}  // namespace std

TEST_F(BlockBuilderTest, PushWhenApplyFails) {
  Extrinsic xt{};
  EXPECT_CALL(*block_builder_api_, apply_extrinsic(xt))
      .WillOnce(Return(outcome::failure(boost::system::error_code{})));

  auto res = block_builder_->pushExtrinsic(xt);
  ASSERT_FALSE(res);
}
