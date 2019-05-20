/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/grandpa_impl.hpp"

#include <gtest/gtest.h>
#include "core/runtime/runtime_test.hpp"
#include "extensions/extension_impl.hpp"
#include "primitives/impl/scale_codec_impl.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"
#include "testutil/runtime/wasm_test.hpp"

using kagome::common::Buffer;
using kagome::extensions::ExtensionImpl;
using kagome::primitives::Digest;
using kagome::primitives::ScaleCodecImpl;
using kagome::runtime::Grandpa;
using kagome::runtime::GrandpaImpl;

using ::testing::_;
using ::testing::Return;

namespace fs = boost::filesystem;

class GrandpaTest : public RuntimeTest {
 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    api_ = std::make_shared<GrandpaImpl>(state_code_, extension_, codec_);
  }

  Digest createDigest() const {
    return Buffer{1, 2, 3};
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
  ASSERT_TRUE(api_->authorities());
}
