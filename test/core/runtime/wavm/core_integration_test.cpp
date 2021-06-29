/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <fstream>

#include "core/runtime/binaryen/runtime_test.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "runtime/binaryen/runtime_api/core_impl.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::host_api::HostApiFactoryImpl;
using kagome::primitives::Block;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::Extrinsic;
using kagome::runtime::Memory;
using kagome::runtime::binaryen::CoreImpl;
using kagome::runtime::binaryen::WasmMemoryImpl;

using ::testing::_;
using ::testing::Return;

namespace fs = boost::filesystem;

class CoreTest : public RuntimeTest {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    RuntimeTest::SetUp();

    auto header_repo = std::make_shared<BlockHeaderRepositoryMock>();
    EXPECT_CALL(*header_repo, getBlockHeader(_))
        .WillRepeatedly(Return(kagome::primitives::BlockHeader{}));
    EXPECT_CALL(*storage_provider_, getLatestRootMock());
    EXPECT_CALL(*storage_provider_, getCurrentBatch());
    EXPECT_CALL(*batch_mock_, get(_));
    EXPECT_CALL(*storage_provider_, rollbackTransaction());

    core_ = std::make_shared<CoreImpl>(
        runtime_env_factory_, wasm_provider_, changes_tracker_, header_repo);
  }

 protected:
  std::shared_ptr<CoreImpl> core_;
};

/**
 * @given initialized core api
 * @when version is invoked
 * @then successful result is returned
 */
TEST_F(CoreTest, VersionTest) {
  ASSERT_TRUE(core_->version());
}

/**
 * @given initialized core api
 * @when execute_block is invoked
 * @then successful result is returned
 */
TEST_F(CoreTest, DISABLED_ExecuteBlockTest) {
  auto block = createBlock();

  ASSERT_TRUE(core_->execute_block(block));
}

/**
 * @given initialised core api
 * @when initialise_block is invoked
 * @then successful result is returned
 */
TEST_F(CoreTest, DISABLED_InitializeBlockTest) {
  auto header = createBlockHeader();

  ASSERT_TRUE(core_->initialise_block(header));
}

/**
 * @given initialized core api
 * @when authorities is invoked
 * @then successful result is returned
 */
TEST_F(CoreTest, DISABLED_AuthoritiesTest) {
  BlockId block_id = 0;
  ASSERT_TRUE(core_->authorities(block_id));
}
