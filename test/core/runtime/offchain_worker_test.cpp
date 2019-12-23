/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "core/runtime/runtime_test.hpp"
#include "extensions/impl/extension_impl.hpp"
#include "primitives/common.hpp"
#include "runtime/impl/offchain_worker_impl.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"

using ::testing::_;
using ::testing::Return;

namespace fs = boost::filesystem;

class OffchainWorkerTest : public RuntimeTest {
  using OffchainWorker = kagome::runtime::OffchainWorker;
  using OffchainWorkerImpl = kagome::runtime::OffchainWorkerImpl;
  using BlockNumber = kagome::primitives::BlockNumber;

 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    api_ = std::make_shared<OffchainWorkerImpl>(wasm_provider_, extension_);
  }

  BlockNumber createBlockNumber() const {
    return BlockNumber{0u};
  }

 protected:
  std::shared_ptr<OffchainWorker> api_;
};

/**
 * @given initialized offchain worker api
 * @when offchain_worker() is invoked
 * @then successful result is returned
 */
TEST_F(OffchainWorkerTest, OffchainWorkerCallSuccess) {
  ASSERT_TRUE(api_->offchain_worker(createBlockNumber()));
}
