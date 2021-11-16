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
      std::shared_ptr<const primitives::CodeSubstituteHashes> code_substitutes,
      std::shared_ptr<application::ChainSpec> chain_spec)
      : storage_{std::move(storage)},
        runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)},
        known_code_substitutes_{std::move(code_substitutes)},
        chain_spec_{std::move(chain_spec)},
        logger_ {log::createLogger("StorageCodeProvider", "runtime")} {
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(runtime_upgrade_tracker_ != nullptr);
    last_state_root_ = storage_->getRootHash();
    auto batch = storage_->getEphemeralBatch();
    if (batch.has_error()) {
      SL_CRITICAL(logger_,
                  "Error getting a batch of the storage: {}",
                  batch.error().message());
    }
    auto res = setCodeFromBatch(*batch.value().get());
    if (res.has_error()) {
      SL_CRITICAL(
          logger_, "Error setting code from batch: {}", res.error().message());
    }
  }

  outcome::result<gsl::span<const uint8_t>> StorageCodeProvider::getCodeAt(
      const storage::trie::RootHash &state) const {
    if (last_state_root_ != state) {
      last_state_root_ = state;

      auto hash = runtime_upgrade_tracker_->getLastCodeUpdateHash(state);
      if (hash.has_value()) {
        if (known_code_substitutes_->count(hash.value())) {
          OUTCOME_TRY(code,
                      chain_spec_->fetchCodeSubstituteByHash(hash.value()));
          OUTCOME_TRY(uncompressCodeIfNeeded(code, cached_code_));
          return cached_code_;
        }
      }
      OUTCOME_TRY(batch, storage_->getEphemeralBatchAt(state));
      OUTCOME_TRY(setCodeFromBatch(*batch.get()));
    }
    return cached_code_;
  }

  outcome::result<void> StorageCodeProvider::setCodeFromBatch(
      const storage::trie::EphemeralTrieBatch &batch) const {
    OUTCOME_TRY(code, batch.get(storage::kRuntimeCodeKey));
    OUTCOME_TRY(uncompressCodeIfNeeded(code, cached_code_));
    return outcome::success();
  }
}  // namespace kagome::runtime
