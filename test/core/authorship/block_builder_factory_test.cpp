/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "authorship/impl/block_builder_factory_impl.hpp"

#include <gtest/gtest.h>
#include "mock/core/blockchain/header_repository_mock.hpp"
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
using kagome::primitives::PreRuntime;
using kagome::runtime::BlockBuilderApiMock;
using kagome::runtime::CoreMock;

class BlockBuilderFactoryTest : public ::testing::Test {
 public:
  void SetUp() override {
    parent_hash_.fill(0);
    parent_id_ = parent_hash_;

    expected_header_.parent_hash = parent_hash_;
    expected_header_.number = expected_number_;
    expected_header_.digest = inherent_digests_;

    EXPECT_CALL(*header_backend_, getNumberByHash(parent_hash_))
        .WillOnce(Return(parent_number_));
  }

  std::shared_ptr<CoreMock> core_ = std::make_shared<CoreMock>();
  std::shared_ptr<BlockBuilderApiMock> block_builder_api_ =
      std::make_shared<BlockBuilderApiMock>();
  std::shared_ptr<HeaderRepositoryMock> header_backend_ =
      std::make_shared<HeaderRepositoryMock>();

  BlockNumber parent_number_{41};
  BlockNumber expected_number_{parent_number_ + 1};
  kagome::common::Hash256 parent_hash_;
  BlockId parent_id_;
  Digest inherent_digests_{{PreRuntime{}}};
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
  auto block_builder_res = factory.create(parent_id_, inherent_digests_);

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
  auto block_builder_res = factory.create(parent_id_, inherent_digests_);

  // then
  ASSERT_FALSE(block_builder_res);
}
