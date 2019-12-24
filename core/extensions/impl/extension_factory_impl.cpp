/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/extension_factory_impl.hpp"

#include "extensions/impl/extension_impl.hpp"

namespace kagome::extensions {

  ExtensionFactoryImpl::ExtensionFactoryImpl(
      std::shared_ptr<storage::trie::TrieDb> db)
      : db_{std::move(db)} {}

  std::shared_ptr<Extension> ExtensionFactoryImpl::createExtension(
      std::shared_ptr<runtime::WasmMemory> memory) const {
    return std::make_shared<ExtensionImpl>(memory, db_);
  }
}  // namespace kagome::extensions
