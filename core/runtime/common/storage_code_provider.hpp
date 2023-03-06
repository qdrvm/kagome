/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP

#include "runtime/runtime_code_provider.hpp"

#include "application/chain_spec.hpp"
#include "log/logger.hpp"

namespace kagome::storage::trie {
  class TrieBatch;
  class TrieStorage;
}  // namespace kagome::storage::trie

namespace kagome::runtime {

  class RuntimeUpgradeTracker;
  using primitives::CodeSubstituteBlockIds;

  class StorageCodeProvider final : public RuntimeCodeProvider {
   public:
    ~StorageCodeProvider() override = default;

    explicit StorageCodeProvider(
        std::shared_ptr<const storage::trie::TrieStorage> storage,
        std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker,
        std::shared_ptr<const CodeSubstituteBlockIds> code_substitutes,
        std::shared_ptr<application::ChainSpec> chain_spec);

    outcome::result<gsl::span<const uint8_t>> getCodeAt(
        const storage::trie::RootHash &state) const override;

   private:
    outcome::result<common::Buffer> setCodeFromBatch(
        const storage::trie::TrieBatch &batch) const;
    std::shared_ptr<const storage::trie::TrieStorage> storage_;
    std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker_;
    std::shared_ptr<const CodeSubstituteBlockIds> known_code_substitutes_;
    std::shared_ptr<application::ChainSpec> chain_spec_;
    mutable common::Buffer cached_code_;
    mutable storage::trie::RootHash last_state_root_;
    log::Logger logger_;
    mutable std::mutex mutex_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP
