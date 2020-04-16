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

#include "runtime/binaryen/runtime_api/core_impl.hpp"

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <fstream>

#include "core/runtime/runtime_test.hpp"
#include "mock/core/storage/trie/trie_db_mock.hpp"
#include "extensions/impl/extension_factory_impl.hpp"
#include "runtime/binaryen/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::extensions::ExtensionFactoryImpl;
using kagome::primitives::Block;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::Extrinsic;
using kagome::runtime::binaryen::CoreImpl;
using kagome::runtime::WasmMemory;
using kagome::runtime::binaryen::WasmMemoryImpl;

using ::testing::_;
using ::testing::Return;

namespace fs = boost::filesystem;

class CoreTest : public RuntimeTest {
 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    core_ = std::make_shared<CoreImpl>(wasm_provider_, extension_factory_);
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
