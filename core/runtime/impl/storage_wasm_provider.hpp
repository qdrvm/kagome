/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP

#include "runtime/wasm_provider.hpp"

#include "storage/trie/trie_db.hpp"

namespace kagome::runtime {

  // key for accessing runtime from storage(hex representation of ":code")
  inline const common::Buffer kRuntimeKey =
      common::Buffer::fromHex("3a636f6465").value();

  class StorageWasmProvider : public WasmProvider {
   public:
    ~StorageWasmProvider() override = default;

    explicit StorageWasmProvider(
        std::shared_ptr<storage::trie::TrieDb> storage);

    const common::Buffer &getStateCode() const override;

   private:
    std::shared_ptr<storage::trie::TrieDb> storage_;
    mutable common::Buffer state_code_;
    mutable common::Buffer last_state_root_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP
