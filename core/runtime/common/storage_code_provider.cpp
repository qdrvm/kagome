/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/storage_code_provider.hpp"

#include <boost/assert.hpp>

#include "log/logger.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/runtime_upgrade_tracker.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime {

  StorageCodeProvider::StorageCodeProvider(
      std::shared_ptr<const storage::trie::TrieStorage> storage,
      std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker,
      const application::CodeSubstitutes &code_substitutes)
      : storage_{std::move(storage)},
        runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)},
        code_substitutes_{code_substitutes} {
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(runtime_upgrade_tracker_ != nullptr);
    last_state_root_ = storage_->getRootHash();
    auto batch = storage_->getEphemeralBatch();
    BOOST_ASSERT_MSG(batch.has_value(), "Error getting a batch of the storage");
    setStateCodeFromBatch(*batch.value().get());
  }

  outcome::result<gsl::span<const uint8_t>> StorageCodeProvider::getCodeAt(
      const storage::trie::RootHash &state) const {
    if (last_state_root_ != state) {
      last_state_root_ = state;

      auto hash = runtime_upgrade_tracker_->getLastCodeUpdateHash(state);
      if (hash.has_value()) {
        if (auto code_it = code_substitutes_.find(hash.value());
            code_it != code_substitutes_.end()) {
          uncompressCodeIfNeeded(code_it->second, cached_code_);
          return cached_code_;
        }
      }
      OUTCOME_TRY(batch, storage_->getEphemeralBatchAt(state));
      setStateCodeFromBatch(*batch.get());
    }
    return cached_code_;
  }

  void StorageCodeProvider::setStateCodeFromBatch(
      const storage::trie::EphemeralTrieBatch &batch) const {
    auto state_code_res = batch.get(storage::kRuntimeCodeKey);
    BOOST_ASSERT_MSG(state_code_res.has_value(),
                     "Runtime code does not exist in the storage");
    uncompressCodeIfNeeded(state_code_res.value(), cached_code_);
  }
}  // namespace kagome::runtime
