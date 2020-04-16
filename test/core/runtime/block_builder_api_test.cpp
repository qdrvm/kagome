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

#include "runtime/binaryen/runtime_api/block_builder_impl.hpp"

#include <gtest/gtest.h>

#include "core/runtime/runtime_test.hpp"
#include "mock/core/storage/trie/trie_db_mock.hpp"
#include "extensions/impl/extension_impl.hpp"
#include "runtime/binaryen/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"

using namespace testing;
using kagome::common::Buffer;
using kagome::extensions::ExtensionImpl;
using kagome::primitives::Extrinsic;
using kagome::primitives::InherentData;
using kagome::runtime::BlockBuilder;
using kagome::runtime::binaryen::BlockBuilderImpl;
using kagome::runtime::binaryen::WasmMemoryImpl;
using kagome::storage::trie::TrieDbMock;

class BlockBuilderApiTest : public RuntimeTest {
 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    builder_ =
        std::make_unique<BlockBuilderImpl>(wasm_provider_, extension_factory_);
  }

 protected:
  std::unique_ptr<BlockBuilder> builder_;
};

/**
 * @given block builder
 * @when calling check_inherents runtime function
 * @then the result of the check is obtained given that the provided arguments
 * were valid
 */
TEST_F(BlockBuilderApiTest, CheckInherents) {
  EXPECT_OUTCOME_FALSE_1(
      builder_->check_inherents(createBlock(), InherentData{}));
}

/**
 * @given block builder
 * @when calling apply_extrinsic runtime function
 * @then the result of the check is obtained given that the provided arguments
 * were valid
 */
TEST_F(BlockBuilderApiTest, ApplyExtrinsic) {
  EXPECT_OUTCOME_FALSE_1(builder_->apply_extrinsic(Extrinsic{Buffer{1, 2, 3}}));
}

/**
 * @given block builder
 * @when calling random_seed runtime function
 * @then the result of the check is obtained given that the provided arguments
 * were valid
 */
TEST_F(BlockBuilderApiTest, DISABLED_RandomSeed) {
  EXPECT_OUTCOME_FALSE_1(builder_->random_seed());
}

/**
 * @given block builder
 * @when calling inherent_extrinsics runtime function
 * @then the result of the check is obtained given that the provided arguments
 * were valid
 */
TEST_F(BlockBuilderApiTest, InherentExtrinsics) {
  EXPECT_OUTCOME_FALSE_1(builder_->inherent_extrinsics(InherentData{}));
}

/**
 * @given block builder
 * @when calling finalize_block runtime function
 * @then the result of the check is obtained given that the provided arguments
 * were valid
 */
TEST_F(BlockBuilderApiTest, FinalizeBlock) {
  EXPECT_OUTCOME_FALSE_1(builder_->finalise_block());
}
