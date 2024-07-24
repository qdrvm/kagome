/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/storage_code_provider.hpp"

#include <boost/assert.hpp>

#include "runtime/runtime_upgrade_tracker.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime {

  StorageCodeProvider::StorageCodeProvider(
      std::shared_ptr<const storage::trie::TrieStorage> storage,
      std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker,
      std::shared_ptr<const primitives::CodeSubstituteBlockIds>
          code_substitutes,
      std::shared_ptr<application::ChainSpec> chain_spec)
      : storage_{std::move(storage)},
        runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)},
        known_code_substitutes_{std::move(code_substitutes)},
        chain_spec_{std::move(chain_spec)},
        logger_{log::createLogger("StorageCodeProvider", "runtime")} {
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(runtime_upgrade_tracker_ != nullptr);
  }

  RuntimeCodeProvider::Result StorageCodeProvider::getCodeAt(
      const storage::trie::RootHash &state) const {
    std::unique_lock lock{mutex_};
    if (last_state_root_ != state) {
      auto block_info =
          runtime_upgrade_tracker_->getLastCodeUpdateBlockInfo(state);
      if (block_info.has_value()) {
        if (known_code_substitutes_->contains(block_info.value())) {
          OUTCOME_TRY(
              code,
              chain_spec_->fetchCodeSubstituteByBlockInfo(block_info.value()));
          return std::make_shared<common::Buffer>(std::move(code));
        }
      }
      OUTCOME_TRY(batch, storage_->getEphemeralBatchAt(state));
      OUTCOME_TRY(code, batch->get(storage::kRuntimeCodeKey));
      cached_code_ = std::make_shared<common::Buffer>(std::move(code));
      last_state_root_ = state;
    }
    return cached_code_;
  }
}  // namespace kagome::runtime
