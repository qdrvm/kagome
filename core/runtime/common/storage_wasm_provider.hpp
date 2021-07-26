/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP

#include "runtime/wasm_provider.hpp"

namespace kagome::storage::trie {
  class EphemeralTrieBatch;
  class TrieStorage;
}  // namespace kagome::storage::trie

namespace kagome::runtime {

  inline const common::Buffer kRuntimeCodeKey = common::Buffer().put(":code");

  class StorageWasmProvider : public WasmProvider {
   public:
    ~StorageWasmProvider() override = default;

    explicit StorageWasmProvider(
        std::shared_ptr<const storage::trie::TrieStorage> storage);

    const common::Buffer &getStateCodeAt(
        const storage::trie::RootHash &at) const override;

   private:
    void setStateCodeFromBatch(
        const outcome::result<
            std::unique_ptr<storage::trie::EphemeralTrieBatch>> &batch) const;
    std::shared_ptr<const storage::trie::TrieStorage> storage_;
    mutable common::Buffer state_code_;
    mutable storage::trie::RootHash last_state_root_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP
