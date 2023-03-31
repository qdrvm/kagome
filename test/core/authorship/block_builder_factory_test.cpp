/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "authorship/impl/block_builder_factory_impl.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/runtime/block_builder_api_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using ::testing::_;
using ::testing::Return;

using kagome::authorship::BlockBuilderFactoryImpl;
using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::Digest;
using kagome::primitives::PreRuntime;
using kagome::runtime::BlockBuilderApiMock;
using kagome::runtime::CoreMock;

class BlockBuilderFactoryTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    parent_hash_.fill(0);
    parent_ = {parent_number_, parent_hash_};

    expected_header_.parent_hash = parent_hash_;
    expected_header_.number = expected_number_;
    expected_header_.digest = inherent_digests_;

    ON_CALL(*header_backend_, getNumberByHash(parent_hash_))
        .WillByDefault(Return(parent_number_));
  }

  std::shared_ptr<CoreMock> core_ = std::make_shared<CoreMock>();
  std::shared_ptr<BlockBuilderApiMock> block_builder_api_ =
      std::make_shared<BlockBuilderApiMock>();
  std::shared_ptr<BlockHeaderRepositoryMock> header_backend_ =
      std::make_shared<BlockHeaderRepositoryMock>();

  BlockNumber parent_number_{41};
  BlockNumber expected_number_{parent_number_ + 1};
  kagome::common::Hash256 parent_hash_;
  kagome::primitives::BlockInfo parent_;
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
  EXPECT_CALL(*core_, initialize_block(expected_header_, _)).WillOnce([] {
    return outcome::success();
  });
  BlockBuilderFactoryImpl factory(core_, block_builder_api_, header_backend_);

  // when
  auto block_builder_res =
      factory.make(parent_, inherent_digests_, std::nullopt);

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
  EXPECT_CALL(*core_, initialize_block(expected_header_, _)).WillOnce([] {
    return boost::system::error_code{};
  });
  BlockBuilderFactoryImpl factory(core_, block_builder_api_, header_backend_);

  // when
  auto block_builder_res =
      factory.make(parent_, inherent_digests_, std::nullopt);

  // then
  ASSERT_FALSE(block_builder_res);
}
