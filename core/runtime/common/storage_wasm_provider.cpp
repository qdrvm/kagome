/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/storage_wasm_provider.hpp"

#include "runtime/common/uncompress_code_if_needed.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime {

  StorageWasmProvider::StorageWasmProvider(
      std::shared_ptr<const storage::trie::TrieStorage> storage)
      : storage_{std::move(storage)} {
    BOOST_ASSERT(storage_ != nullptr);

    last_state_root_ = storage_->getRootHash();
    auto batch = storage_->getEphemeralBatch();
    setStateCodeFromBatch(batch);
  }

  const common::Buffer &StorageWasmProvider::getStateCodeAt(
      const storage::trie::RootHash &at) const {
    if (last_state_root_ == at) {
      return state_code_;
    }
    last_state_root_ = at;

    auto batch = storage_->getEphemeralBatchAt(at);
    setStateCodeFromBatch(batch);
    return state_code_;
  }

  void StorageWasmProvider::setStateCodeFromBatch(
      const outcome::result<std::unique_ptr<storage::trie::EphemeralTrieBatch>>
          &batch) const {
    BOOST_ASSERT_MSG(batch.has_value(), "Error getting a batch of the storage");
    auto state_code_res = batch.value()->get(kRuntimeCodeKey);
    BOOST_ASSERT_MSG(state_code_res.has_value(),
                     "Runtime code does not exist in the storage");
    uncompressCodeIfNeeded(state_code_res.value(), state_code_);
  }

}  // namespace kagome::runtime
