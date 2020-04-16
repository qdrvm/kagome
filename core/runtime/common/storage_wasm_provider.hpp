/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
