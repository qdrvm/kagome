/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/storage_code_provider.hpp"

#include "storage/trie/trie_storage.hpp"
#include "log/logger.hpp"

namespace kagome::runtime {

  StorageCodeProvider::StorageCodeProvider(
      std::shared_ptr<const storage::trie::TrieStorage> storage)
      : storage_{std::move(storage)} {
    BOOST_ASSERT(storage_ != nullptr);

    last_state_root_ = storage_->getRootHash();
    auto batch = storage_->getEphemeralBatch();
    BOOST_ASSERT_MSG(batch.has_value(), "Error getting a batch of the storage");
    auto log = kagome::log::createLogger("Code Provider");
    auto cursor = batch.value()->trieCursor();
    cursor->next();
    size_t elements_num {};
    while (cursor->isValid()) {
      log->info(cursor->key() ? cursor->key().value().toString() : "no key");
      cursor->next();
      elements_num++;
    }
    log->info("Trie size: {}", elements_num);
    auto state_code_res = batch.value()->get(kRuntimeCodeKey);
    state_code_ = state_code_res.value();
  }

  outcome::result<gsl::span<const uint8_t>> StorageCodeProvider::getCodeAt(
      const storage::trie::RootHash &at) const {
    auto current_state_root = storage_->getRootHash();
    if (last_state_root_ == current_state_root) {
      return state_code_;
    }
    last_state_root_ = current_state_root;

    auto batch = storage_->getEphemeralBatch();
    BOOST_ASSERT_MSG(batch.has_value(), "Error getting a batch of the storage");
    auto state_code_res = batch.value()->get(kRuntimeCodeKey);
    BOOST_ASSERT_MSG(state_code_res.has_value(),
                     "Runtime code does not exist in the storage");
    state_code_ = state_code_res.value();
    return state_code_;
  }

}  // namespace kagome::runtime
