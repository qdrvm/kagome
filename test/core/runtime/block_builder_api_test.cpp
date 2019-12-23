/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "core/runtime/runtime_test.hpp"
#include "core/storage/trie/mock_trie_db.hpp"
#include "extensions/impl/extension_impl.hpp"
#include "runtime/impl/block_builder_api_impl.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"

using namespace testing;
using kagome::common::Buffer;
using kagome::extensions::ExtensionImpl;
using kagome::primitives::Extrinsic;
using kagome::primitives::InherentData;
using kagome::runtime::BlockBuilderApi;
using kagome::runtime::BlockBuilderApiImpl;
using kagome::runtime::WasmMemoryImpl;
using kagome::storage::trie::MockTrieDb;

class BlockBuilderApiTest : public RuntimeTest {
 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    builder_ =
        std::make_unique<BlockBuilderApiImpl>(wasm_provider_, extension_);
  }

 protected:
  std::unique_ptr<BlockBuilderApi> builder_;
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
