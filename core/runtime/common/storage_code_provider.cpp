/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/storage_code_provider.hpp"

#include <boost/assert.hpp>

#include "log/logger.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime {

  StorageCodeProvider::StorageCodeProvider(
      std::shared_ptr<const storage::trie::TrieStorage> storage)
      : storage_{std::move(storage)} {
    BOOST_ASSERT(storage_ != nullptr);
    last_state_root_ = storage_->getRootHash();
    auto batch = storage_->getEphemeralBatch();
    BOOST_ASSERT_MSG(batch.has_value(), "Error getting a batch of the storage");
    auto state_code_res = batch.value()->get(kRuntimeCodeKey);
    cached_code_ = state_code_res.value();
  }

  outcome::result<gsl::span<const uint8_t>>
  StorageCodeProvider::getCodeAt(const storage::trie::RootHash &state) const {
    if (last_state_root_ != state) {
      last_state_root_ = state;

      auto batch = storage_->getEphemeralBatch();
      BOOST_ASSERT_MSG(batch.has_value(),
                       "Error getting a batch of the storage");
      auto state_code_res = batch.value()->get(kRuntimeCodeKey);
      BOOST_ASSERT_MSG(state_code_res.has_value(),
                       "Runtime code does not exist in the storage");
      cached_code_ = state_code_res.value();
    }
    return cached_code_;
  }

}  // namespace kagome::runtime
