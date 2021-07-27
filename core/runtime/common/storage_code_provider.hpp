/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP

#include "runtime/runtime_code_provider.hpp"

#include "log/logger.hpp"

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::runtime {

  inline const common::Buffer kRuntimeCodeKey = common::Buffer().put(":code");

  class StorageCodeProvider final : public RuntimeCodeProvider {
   public:
    ~StorageCodeProvider() override = default;

    explicit StorageCodeProvider(
        std::shared_ptr<const storage::trie::TrieStorage> storage);

    outcome::result<gsl::span<const uint8_t>> getCodeAt(
        const storage::trie::RootHash &state) const override;

   private:
    std::shared_ptr<const storage::trie::TrieStorage> storage_;
    mutable common::Buffer cached_code_;
    mutable storage::trie::RootHash last_state_root_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP
