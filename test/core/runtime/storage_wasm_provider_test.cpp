/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/storage_wasm_provider.hpp"

#include <gtest/gtest.h>

#include "core/storage/trie/mock_trie_db.hpp"

using namespace kagome;  // NOLINT

using ::testing::Return;

class StorageWasmProviderTest : public ::testing::Test {
 public:
 protected:
  common::Buffer state_code_{1, 3, 3, 7};
};

/**
 * @given storage with "first_state_root" as merkle hash and "state_code" stored
 * by runtime key @and wasm provider initialized with this storage
 * @when state code is obtained by wasm provider
 * @then obtained state code and "state_code" are equal
 */
TEST_F(StorageWasmProviderTest, GetCodeWhenNoStorageUpdates) {
  auto trie_db = std::make_shared<storage::trie::MockTrieDb>();
  common::Buffer first_state_root{1, 1, 1, 1};

  // given
  EXPECT_CALL(*trie_db, getRootHash()).WillOnce(Return(first_state_root));
  EXPECT_CALL(*trie_db, get(runtime::kRuntimeKey))
      .WillOnce(Return(state_code_));
  auto wasm_provider = std::make_shared<runtime::StorageWasmProvider>(trie_db);

  EXPECT_CALL(*trie_db, getRootHash()).WillOnce(Return(first_state_root));

  // when
  auto obtained_state_code = wasm_provider->getStateCode();

  // then
  ASSERT_EQ(obtained_state_code, state_code_);
}

/**
 * @given storage with "first_state_root" as merkle hash and "state_code" stored
 * by runtime key @and wasm provider initialized with this storage
 * @when storage root is updated by "second_state_root" and "new_state_code" is
 * put into the storage @and state code is obtained by wasm provider
 * @then obtained state code and "new_state_code" are equal
 */
TEST_F(StorageWasmProviderTest, GetCodeWhenStorageUpdates) {
  auto trie_db = std::make_shared<storage::trie::MockTrieDb>();
  common::Buffer first_state_root{1, 1, 1, 1};
  common::Buffer second_state_root{2, 2, 2, 2};

  // given
  EXPECT_CALL(*trie_db, getRootHash()).WillOnce(Return(first_state_root));
  EXPECT_CALL(*trie_db, get(runtime::kRuntimeKey))
      .WillOnce(Return(state_code_));
  auto wasm_provider = std::make_shared<runtime::StorageWasmProvider>(trie_db);

  common::Buffer new_state_code{1, 3, 3, 8};
  EXPECT_CALL(*trie_db, getRootHash()).WillOnce(Return(second_state_root));
  EXPECT_CALL(*trie_db, get(runtime::kRuntimeKey))
      .WillOnce(Return(new_state_code));

  // when
  auto obtained_state_code = wasm_provider->getStateCode();

  // then
  ASSERT_EQ(obtained_state_code, new_state_code);
}
