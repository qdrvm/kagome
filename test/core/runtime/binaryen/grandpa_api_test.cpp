/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/grandpa_api.hpp"

#include <gtest/gtest.h>

#include "core/runtime/binaryen/binaryen_runtime_test.hpp"
#include "host_api/impl/host_api_impl.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"

using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::host_api::HostApiImpl;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::Digest;
using kagome::primitives::PreRuntime;
using kagome::runtime::GrandpaApi;
using kagome::runtime::GrandpaApiImpl;

using ::testing::_;
using ::testing::Return;

namespace fs = boost::filesystem;

class GrandpaTest : public BinaryenRuntimeTest {
 public:
  void SetUp() override {
    BinaryenRuntimeTest::SetUp();

    api_ = std::make_shared<GrandpaApiImpl>(
        std::make_shared<BlockHeaderRepositoryMock>(), executor_);
  }

  Digest createDigest() const {
    return Digest{PreRuntime{}};
  }

  BlockId createBlockId() const {
    return BlockId(BlockNumber{0});
  }

 protected:
  std::shared_ptr<GrandpaApi> api_;
};

// TODO(yuraz): PRE-157 find out do we need to give block_id to api functions
/**
 * @given initialized Grandpa api
 * @when pendingChange() is invoked
 * @then successful result is returned
 */
TEST_F(GrandpaTest, DISABLED_PendingChange) {
  auto &&digest = createDigest();
  ASSERT_TRUE(api_->pending_change("block_hash"_hash256, digest));
}

/**
 * @given initialized Grandpa api
 * @when pendingChange() is invoked
 * @then successful result is returned
 */
TEST_F(GrandpaTest, DISABLED_ForcedChange) {
  auto &&digest = createDigest();
  ASSERT_TRUE(api_->forced_change("block_hash"_hash256, digest));
}

/**
 * @given initialized Grandpa api
 * @when authorities() is invoked
 * @then successful result is returned
 * @brief writes "Uninteresting mock function call - returning default value"
 */
TEST_F(GrandpaTest, DISABLED_Authorities) {
  auto block_id = createBlockId();
  ASSERT_TRUE(api_->authorities(block_id));
}
