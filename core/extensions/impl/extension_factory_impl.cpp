/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/extension_factory_impl.hpp"

#include "extensions/impl/extension_impl.hpp"

namespace kagome::extensions {

  ExtensionFactoryImpl::ExtensionFactoryImpl(
      std::shared_ptr<storage::changes_trie::ChangesTracker> tracker)
      : changes_tracker_{std::move(tracker)} {
    BOOST_ASSERT(changes_tracker_ != nullptr);
  }

  std::shared_ptr<Extension> ExtensionFactoryImpl::createExtension(
      std::shared_ptr<runtime::WasmMemory> memory,
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider) const {
    return std::make_shared<ExtensionImpl>(
        memory, storage_provider, changes_tracker_);
  }
}  // namespace kagome::extensions
