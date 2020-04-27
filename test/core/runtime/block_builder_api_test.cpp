/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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

    builder_ = std::make_unique<BlockBuilderImpl>(runtime_manager_);
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
