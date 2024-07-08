/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <fstream>

#include "core/runtime/wavm/wavm_runtime_test.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "runtime/wavm/instance_environment_factory.hpp"
#include "runtime/wavm/memory_impl.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::host_api::HostApiFactoryImpl;
using kagome::primitives::Block;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::Extrinsic;
using kagome::runtime::CoreImpl;
using kagome::runtime::Memory;
using kagome::runtime::wavm::MemoryImpl;

using ::testing::_;
using ::testing::Return;

namespace fs = kagome::filesystem;

class CoreTest : public ::testing::Test, public WavmRuntimeTest {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    SetUpImpl();

    core_ =
        std::make_shared<CoreImpl>(executor_, nullptr, header_repo_, nullptr);
  }

 protected:
  std::shared_ptr<CoreImpl> core_;
};

/**
 * @given initialized core api
 * @when execute_block is invoked
 * @then successful result is returned
 */
TEST_F(CoreTest, DISABLED_ExecuteBlockTest) {
  auto block = createBlock("block_hash"_hash256, 42);

  ASSERT_TRUE(core_->execute_block(block, std::nullopt));
}

/**
 * @given initialised core api
 * @when initialize_block is invoked
 * @then successful result is returned
 */
TEST_F(CoreTest, DISABLED_InitializeBlockTest) {
  auto header = createBlockHeader("block_hash"_hash256, 42);

  ASSERT_TRUE(core_->initialize_block(header, std::nullopt));
}
