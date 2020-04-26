/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTENSIONS_IMPL_EXTENSION_FACTORY_IMPL_HPP
#define KAGOME_CORE_EXTENSIONS_IMPL_EXTENSION_FACTORY_IMPL_HPP

#include "extensions/extension_factory.hpp"

#include "blockchain/changes_trie_builder.hpp"
#include "storage/trie_db_overlay/trie_db_overlay.hpp"

namespace kagome::extensions {

  class ExtensionFactoryImpl : public ExtensionFactory {
   public:
    ~ExtensionFactoryImpl() override = default;
    ExtensionFactoryImpl(
        std::shared_ptr<storage::trie_db_overlay::TrieDbOverlay> db,
        std::shared_ptr<blockchain::ChangesTrieBuilder> builder);

    std::shared_ptr<Extension> createExtension(
        std::shared_ptr<runtime::WasmMemory> memory) const override;

   private:
    std::shared_ptr<storage::trie_db_overlay::TrieDbOverlay> db_;
    std::shared_ptr<blockchain::ChangesTrieBuilder> changes_trie_builder_;
  };

}  // namespace kagome::extensions

#endif  // KAGOME_CORE_EXTENSIONS_IMPL_EXTENSION_FACTORY_IMPL_HPP
