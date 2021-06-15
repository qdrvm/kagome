/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/storage_code_provider.hpp"

#include <gtest/gtest.h>

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;  // NOLINT

using ::testing::Invoke;
using ::testing::Return;
using ::testing::_;

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
  storage::trie::RootHash first_state_root{{1, 1, 1, 1}};

  // given
  EXPECT_CALL(*trie_db, getRootHashMock()).WillOnce(Return(first_state_root));
  EXPECT_CALL(*trie_db, getEphemeralBatch()).WillOnce(Invoke([this]() {
    auto batch = std::make_unique<storage::trie::EphemeralTrieBatchMock>();
    EXPECT_CALL(*batch, get(runtime::kRuntimeCodeKey))
        .WillOnce(Return(state_code_));
    return batch;
  }));
  auto wasm_provider = std::make_shared<runtime::StorageCodeProvider>(trie_db);

  // when
  EXPECT_OUTCOME_TRUE(obtained_state_code,
                      wasm_provider->getCodeAt(first_state_root));

  // then
  EXPECT_THAT(gsl::span<const uint8_t>(state_code_),
              testing::ContainerEq(obtained_state_code));
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
  storage::trie::RootHash first_state_root{{1, 1, 1, 1}};
  storage::trie::RootHash second_state_root{{2, 2, 2, 2}};

  // given
  EXPECT_CALL(*trie_db, getRootHashMock()).WillOnce(Return(first_state_root));
  EXPECT_CALL(*trie_db, getEphemeralBatch()).WillOnce(Invoke([this]() {
    auto batch = std::make_unique<storage::trie::EphemeralTrieBatchMock>();
    EXPECT_CALL(*batch, get(runtime::kRuntimeCodeKey))
        .WillOnce(Return(state_code_));
    return batch;
  }));
  auto wasm_provider = std::make_shared<runtime::StorageCodeProvider>(trie_db);

  common::Buffer new_state_code{{1, 3, 3, 8}};
  EXPECT_CALL(*trie_db, getEphemeralBatch())
      .WillOnce(Invoke([&new_state_code]() {
        auto batch = std::make_unique<storage::trie::EphemeralTrieBatchMock>();
        EXPECT_CALL(*batch, get(runtime::kRuntimeCodeKey))
            .WillOnce(Return(new_state_code));
        return batch;
      }));

  // when
  EXPECT_OUTCOME_TRUE(obtained_state_code,
                      wasm_provider->getCodeAt(second_state_root));

  // then
  EXPECT_THAT(gsl::span<const uint8_t>(new_state_code),
              testing::ContainerEq(obtained_state_code));
}
