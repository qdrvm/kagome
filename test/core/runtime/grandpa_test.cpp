/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/grandpa_impl.hpp"

#include <gtest/gtest.h>

#include "core/runtime/runtime_test.hpp"
#include "extensions/impl/extension_impl.hpp"
#include "runtime/common/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::extensions::ExtensionImpl;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::Digest;
using kagome::runtime::Grandpa;
using kagome::runtime::binaryen::GrandpaImpl;

using ::testing::_;
using ::testing::Return;

namespace fs = boost::filesystem;

class GrandpaTest : public RuntimeTest {
 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    api_ = std::make_shared<GrandpaImpl>(wasm_provider_, extension_factory_);
  }

  Digest createDigest() const {
    return Buffer{1, 2, 3};
  }

  BlockId createBlockId() const {
    return BlockId(BlockNumber{0});
  }

 protected:
  std::shared_ptr<Grandpa> api_;
};

// TODO(yuraz): PRE-157 find out do we need to give block_id to api functions
/**
 * @given initialized Grandpa api
 * @when pendingChange() is invoked
 * @then successful result is returned
 */
TEST_F(GrandpaTest, DISABLED_PendingChange) {
  auto &&digest = createDigest();
  ASSERT_TRUE(api_->pending_change(digest));
}

/**
 * @given initialized Grandpa api
 * @when pendingChange() is invoked
 * @then successful result is returned
 */
TEST_F(GrandpaTest, DISABLED_ForcedChange) {
  auto &&digest = createDigest();
  ASSERT_TRUE(api_->forced_change(digest));
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
