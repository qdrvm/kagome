/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "core/runtime/binaryen/binaryen_runtime_test.hpp"
#include "host_api/impl/host_api_impl.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "runtime/binaryen/memory_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/runtime_api/impl/block_builder.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Buffer;
using kagome::host_api::HostApiImpl;
using kagome::primitives::BlockInfo;
using kagome::primitives::Extrinsic;
using kagome::primitives::InherentData;
using kagome::runtime::BlockBuilder;
using kagome::runtime::BlockBuilderImpl;
using kagome::runtime::binaryen::MemoryImpl;
using kagome::storage::trie::TrieStorageMock;
using testing::_;
using testing::Return;

class BlockBuilderApiTest : public BinaryenRuntimeTest {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    BinaryenRuntimeTest::SetUp();

    builder_ = std::make_unique<BlockBuilderImpl>(executor_);
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
  SCOPED_TRACE("CheckInherents");
  EXPECT_OUTCOME_TRUE_1(builder_->check_inherents(
      createBlock("block_42"_hash256, 42), InherentData{}))
}

/**
 * @given block builder
 * @when calling apply_extrinsic runtime function
 * @then the result of the check is obtained given that the provided arguments
 * were valid
 */
TEST_F(BlockBuilderApiTest, ApplyExtrinsic) {
  preparePersistentStorageExpects();
  createBlock("block_hash_43"_hash256, 43);
  EXPECT_OUTCOME_FALSE_1(builder_->apply_extrinsic(BlockInfo{43, "block_hash_43"_hash256},
                                "state_root"_hash256,
                                Extrinsic{Buffer{1, 2, 3}}));
}

/**
 * @given block builder
 * @when calling random_seed runtime function
 * @then the result of the check is obtained given that the provided arguments
 * were valid
 */
TEST_F(BlockBuilderApiTest, DISABLED_RandomSeed){
    EXPECT_OUTCOME_FALSE_1(builder_->random_seed("block_hash"_hash256))}

/**
 * @given block builder
 * @when calling inherent_extrinsics runtime function
 * @then the result of the check is obtained given that the provided arguments
 * were valid
 */
TEST_F(BlockBuilderApiTest, InherentExtrinsics) {
  prepareEphemeralStorageExpects();
  createBlock("block_hash_44"_hash256, 44);
  EXPECT_OUTCOME_FALSE_1(
      builder_->inherent_extrinsics(BlockInfo{44, "block_hash_44"_hash256},
                                    "state_root"_hash256,
                                    InherentData{}))
}

/**
 * @given block builder
 * @when calling finalize_block runtime function
 * @then the result of the check is obtained given that the provided arguments
 * were valid
 */
TEST_F(BlockBuilderApiTest, DISABLED_FinalizeBlock) {
  preparePersistentStorageExpects();
  createBlock("block_hash"_hash256, 42);
  EXPECT_OUTCOME_FALSE_1(builder_->finalize_block(
      BlockInfo{42, "block_hash"_hash256}, "state_root"_hash256))
}
