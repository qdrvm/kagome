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

#include "runtime/binaryen/runtime_api/grandpa_impl.hpp"

#include <gtest/gtest.h>

#include "core/runtime/runtime_test.hpp"
#include "extensions/impl/extension_impl.hpp"
#include "runtime/binaryen/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::extensions::ExtensionImpl;
using kagome::primitives::BlockId;
using kagome::primitives::PreRuntime;
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
    return Digest{PreRuntime{}};
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
