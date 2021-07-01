/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/storage_wasm_provider.hpp"

#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime {

  StorageWasmProvider::StorageWasmProvider(
      std::shared_ptr<const storage::trie::TrieStorage> storage)
      : storage_{std::move(storage)} {
    BOOST_ASSERT(storage_ != nullptr);

    last_state_root_ = storage_->getRootHash();
    auto batch = storage_->getEphemeralBatch();
    BOOST_ASSERT_MSG(batch.has_value(), "Error getting a batch of the storage");
    auto state_code_res = batch.value()->get(kRuntimeCodeKey);
    BOOST_ASSERT_MSG(state_code_res.has_value(),
                     "Runtime code does not exist in the storage");
    state_code_ = state_code_res.value();
  }

  const common::Buffer &StorageWasmProvider::getStateCodeAt(
      const storage::trie::RootHash &at) const {
    if (last_state_root_ == at) {
      return state_code_;
    }
    last_state_root_ = at;

    auto batch = storage_->getEphemeralBatchAt(at);
    BOOST_ASSERT_MSG(batch.has_value(), "Error getting a batch of the storage");
    auto state_code_res = batch.value()->get(kRuntimeCodeKey);
    BOOST_ASSERT_MSG(state_code_res.has_value(),
                     "Runtime code does not exist in the storage");
    state_code_ = state_code_res.value();
    return state_code_;
  }

}  // namespace kagome::runtime
