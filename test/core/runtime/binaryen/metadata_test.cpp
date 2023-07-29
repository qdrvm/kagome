/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/grandpa_api.hpp"

#include <gtest/gtest.h>

#include "core/runtime/binaryen/binaryen_runtime_test.hpp"
#include "host_api/impl/host_api_impl.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "runtime/binaryen/memory_impl.hpp"
#include "runtime/runtime_api/impl/metadata.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using ::testing::_;
using ::testing::Return;

using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockInfo;
using kagome::runtime::Metadata;
using kagome::runtime::MetadataImpl;

namespace fs = kagome::filesystem;

class MetadataTest : public BinaryenRuntimeTest {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    BinaryenRuntimeTest::SetUp();
    prepareEphemeralStorageExpects();

    api_ = std::make_shared<MetadataImpl>(executor_);
  }

 protected:
  std::shared_ptr<Metadata> api_;
};

/**
 * @given initialized Metadata api
 * @when metadata() is invoked
 * @then successful result is returned
 */
TEST_F(MetadataTest, metadata) {
  EXPECT_CALL(*header_repo_, getBlockHeader("block_hash"_hash256))
      .WillRepeatedly(Return(BlockHeader{.number = 42}));
  ASSERT_TRUE(api_->metadata("block_hash"_hash256));
}
