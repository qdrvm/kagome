/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/grandpa_api.hpp"

#include <gtest/gtest.h>

#include "core/runtime/binaryen/binaryen_runtime_test.hpp"
#include "host_api/impl/host_api_impl.hpp"
#include "mock/core/runtime/runtime_upgrade_tracker_mock.hpp"
#include "runtime/binaryen/memory_impl.hpp"
#include "runtime/runtime_api/impl/metadata.hpp"
#include "testutil/prepare_loggers.hpp"

using ::testing::_;
using ::testing::Return;

using kagome::blockchain::BlockTreeMock;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockInfo;
using kagome::runtime::Metadata;
using kagome::runtime::MetadataImpl;
using kagome::runtime::RuntimeUpgradeTrackerMock;

namespace fs = kagome::filesystem;

class MetadataTest : public BinaryenRuntimeTest {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    BinaryenRuntimeTest::SetUp();
    prepareEphemeralStorageExpects();

    api_ = std::make_shared<MetadataImpl>(
        executor_, block_tree_, runtime_upgrade_tracker_);
  }

 protected:
  std::shared_ptr<Metadata> api_;
  std::shared_ptr<RuntimeUpgradeTrackerMock> runtime_upgrade_tracker_ =
      std::make_shared<RuntimeUpgradeTrackerMock>();
};

/**
 * @given initialized Metadata api
 * @when metadata() is invoked
 * @then successful result is returned
 */
TEST_F(MetadataTest, metadata) {
  BlockInfo info{42, "block_hash"_hash256};
  EXPECT_CALL(*block_tree_, getBlockHeader(info.hash))
      .WillRepeatedly(Return(BlockHeader{info.number, {}, {}, {}, {}}));
  EXPECT_CALL(*block_tree_, getNumberByHash(info.hash))
      .WillOnce(Return(info.number));
  EXPECT_CALL(*runtime_upgrade_tracker_, getLastCodeUpdateState(info))
      .WillOnce(Return(info.hash));
  ASSERT_TRUE(api_->metadata(info.hash));
}
