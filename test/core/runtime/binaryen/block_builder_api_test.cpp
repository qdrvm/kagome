/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "core/runtime/binaryen/runtime_test.hpp"
#include "host_api/impl/host_api_impl.hpp"
#include "mock/core/host_api/host_api_factory_mock.hpp"
#include "mock/core/runtime/wasm_provider_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "runtime/binaryen/runtime_api/block_builder_impl.hpp"
#include "runtime/binaryen/wasm_memory_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace testing;
using kagome::common::Buffer;
using kagome::host_api::HostApiImpl;
using kagome::primitives::Extrinsic;
using kagome::primitives::InherentData;
using kagome::runtime::BlockBuilder;
using kagome::runtime::binaryen::BlockBuilderImpl;
using kagome::runtime::binaryen::WasmMemoryImpl;
using kagome::storage::trie::TrieStorageMock;

class BlockBuilderApiTest : public RuntimeTest {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    RuntimeTest::SetUp();

    builder_ = std::make_unique<BlockBuilderImpl>(runtime_env_factory_);
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
  prepareEphemeralStorageExpects();
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
  preparePersistentStorageExpects();
  EXPECT_CALL(*batch_mock_, batchOnTop());
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
  prepareEphemeralStorageExpects();
  EXPECT_OUTCOME_FALSE_1(builder_->inherent_extrinsics(InherentData{}));
}

/**
 * @given block builder
 * @when calling finalize_block runtime function
 * @then the result of the check is obtained given that the provided arguments
 * were valid
 */
TEST_F(BlockBuilderApiTest, DISABLED_FinalizeBlock) {
  EXPECT_OUTCOME_FALSE_1(builder_->finalise_block());
}
