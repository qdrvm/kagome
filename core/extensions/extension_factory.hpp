/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTENSIONS_EXTENSION_FACTORY_HPP
#define KAGOME_CORE_EXTENSIONS_EXTENSION_FACTORY_HPP

#include "extensions/extension.hpp"

#include "runtime/trie_storage_provider.hpp"

namespace kagome::extensions {

  /**
   * Creates extension containing provided wasm memory
   */
  class ExtensionFactory {
   public:
    virtual ~ExtensionFactory() = default;

    /**
     * Takes \param memory and creates \return extension using this memory
     */
    virtual std::shared_ptr<Extension> createExtension(
        std::shared_ptr<runtime::WasmMemory> memory,
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider) const = 0;
  };

}  // namespace kagome::extensions

#endif  // KAGOME_CORE_EXTENSIONS_EXTENSION_FACTORY_HPP
