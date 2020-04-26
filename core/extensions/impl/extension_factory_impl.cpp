/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/extension_factory_impl.hpp"

#include "extensions/impl/extension_impl.hpp"

namespace kagome::extensions {

  ExtensionFactoryImpl::ExtensionFactoryImpl(
      std::shared_ptr<storage::trie_db_overlay::TrieDbOverlay> db,
      std::shared_ptr<blockchain::ChangesTrieBuilder> builder)
      : db_{std::move(db)}, changes_trie_builder_{std::move(builder)} {
        BOOST_ASSERT(db_ != nullptr);
        BOOST_ASSERT(changes_trie_builder_ != nullptr);
      }

  std::shared_ptr<Extension> ExtensionFactoryImpl::createExtension(
      std::shared_ptr<runtime::WasmMemory> memory) const {
    return std::make_shared<ExtensionImpl>(memory, db_, changes_trie_builder_);
  }
}  // namespace kagome::extensions
