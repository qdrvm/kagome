/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/storage_code_provider.hpp"

#include <gtest/gtest.h>

#include "mock/core/application/chain_spec_mock.hpp"
#include "mock/core/runtime/runtime_upgrade_tracker_mock.hpp"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "storage/predefined_keys.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;  // NOLINT

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

class StorageCodeProviderTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    state_code_ = common::Buffer{1, 3, 3, 7};
  }

 protected:
  common::Buffer state_code_;
};

/**
 * @given storage with "first_state_root" as merkle hash and "state_code" stored
 * by runtime key @and wasm provider initialized with this storage
 * @when state code is obtained by wasm provider
 * @then obtained state code and "state_code" are equal
 */
TEST_F(StorageCodeProviderTest, GetCodeWhenNoStorageUpdates) {
  // TODO(Harrm): fix it for the new upgrade logic
  auto trie_db = std::make_shared<storage::trie::TrieStorageMock>();
  auto tracker = std::make_shared<runtime::RuntimeUpgradeTrackerMock>();
  storage::trie::RootHash first_state_root{{1, 1, 1, 1}};
  primitives::BlockInfo block_info(11, primitives::BlockHash{{11, 11, 11, 11}});

  // given
  EXPECT_CALL(*trie_db, getEphemeralBatchAt(first_state_root))
      .WillOnce(Invoke([this]() {
        auto batch = std::make_unique<storage::trie::TrieBatchMock>();
        EXPECT_CALL(*batch,
                    getMock(common::BufferView{storage::kRuntimeCodeKey}))
            .WillOnce(Return(state_code_));
        return batch;
      }));
  EXPECT_CALL(*tracker, getLastCodeUpdateBlockInfo(first_state_root))
      .WillOnce(Return(block_info));

  auto wasm_provider = std::make_shared<runtime::StorageCodeProvider>(
      trie_db,
      tracker,
      std::make_shared<primitives::CodeSubstituteBlockIds>(),
      std::make_shared<application::ChainSpecMock>());

  // when
  EXPECT_OUTCOME_TRUE(obtained_state_code,
                      wasm_provider->getCodeAt(first_state_root));

  // then
  ASSERT_EQ(obtained_state_code,
            gsl::span<const uint8_t>(state_code_.data(),
                                     state_code_.data() + state_code_.size()));
}

/**
 * @given storage with "first_state_root" as merkle hash and "state_code" stored
 * by runtime key @and wasm provider initialized with this storage
 * @when storage root is updated by "second_state_root" and "new_state_code" is
 * put into the storage @and state code is obtained by wasm provider
 * @then obtained state code and "new_state_code" are equal
 */
TEST_F(StorageCodeProviderTest, DISABLED_GetCodeWhenStorageUpdates) {
  // TODO(Harrm): fix it for the new upgrade logic
  auto trie_db = std::make_shared<storage::trie::TrieStorageMock>();
  auto tracker = std::make_shared<runtime::RuntimeUpgradeTrackerMock>();
  storage::trie::RootHash first_state_root{{1, 1, 1, 1}};
  storage::trie::RootHash second_state_root{{2, 2, 2, 2}};

  // given
  EXPECT_CALL(*trie_db, getEphemeralBatchAt(first_state_root))
      .WillOnce(Invoke([this]() {
        auto batch = std::make_unique<storage::trie::TrieBatchMock>();
        EXPECT_CALL(*batch,
                    getMock(common::BufferView{storage::kRuntimeCodeKey}))
            .WillOnce(Return(state_code_));
        return batch;
      }));
  auto wasm_provider = std::make_shared<runtime::StorageCodeProvider>(
      trie_db,
      tracker,
      std::make_shared<primitives::CodeSubstituteBlockIds>(),
      std::make_shared<application::ChainSpecMock>());

  common::Buffer new_state_code{{1, 3, 3, 8}};
  EXPECT_CALL(*trie_db, getEphemeralBatchAt(second_state_root))
      .WillOnce(Invoke([&new_state_code](auto &) {
        auto batch = std::make_unique<storage::trie::TrieBatchMock>();
        EXPECT_CALL(*batch,
                    getMock(common::BufferView{storage::kRuntimeCodeKey}))
            .WillOnce(Return(new_state_code));
        return batch;
      }));

  // when
  EXPECT_OUTCOME_TRUE(obtained_state_code,
                      wasm_provider->getCodeAt(second_state_root));

  // then
  ASSERT_EQ(obtained_state_code, gsl::span<const uint8_t>(state_code_));
}
