/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTENSIONS_IMPL_EXTENSION_FACTORY_IMPL_HPP
#define KAGOME_CORE_EXTENSIONS_IMPL_EXTENSION_FACTORY_IMPL_HPP

#include "extensions/extension_factory.hpp"
#include "storage/trie/trie_db.hpp"

namespace kagome::extensions {

  class ExtensionFactoryImpl : public ExtensionFactory {
   public:
    ~ExtensionFactoryImpl() override = default;
    explicit ExtensionFactoryImpl(std::shared_ptr<storage::trie::TrieDb> db);

    std::shared_ptr<Extension> createExtension(
        std::shared_ptr<runtime::WasmMemory> memory) const override;

   private:
    std::shared_ptr<storage::trie::TrieDb> db_;
  };

}  // namespace kagome::extensions

#endif  // KAGOME_CORE_EXTENSIONS_IMPL_EXTENSION_FACTORY_IMPL_HPP
