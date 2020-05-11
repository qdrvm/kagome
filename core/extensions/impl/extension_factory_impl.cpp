/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/extension_factory_impl.hpp"

#include "extensions/impl/extension_impl.hpp"

namespace kagome::extensions {

  ExtensionFactoryImpl::ExtensionFactoryImpl(
      std::shared_ptr<storage::changes_trie::ChangesTrieBuilder> builder,
      std::shared_ptr<storage::changes_trie::ChangesTracker> tracker)
      : changes_trie_builder_{std::move(builder)},
        changes_tracker_{std::move(tracker)} {
    BOOST_ASSERT(changes_trie_builder_ != nullptr);
    BOOST_ASSERT(changes_tracker_ != nullptr);
  }

  std::shared_ptr<Extension> ExtensionFactoryImpl::createExtension(
      std::shared_ptr<runtime::WasmMemory> memory,
      std::shared_ptr<storage::trie::TrieBatch> storage_batch) const {
    return std::make_shared<ExtensionImpl>(
        memory, storage_batch, changes_trie_builder_, changes_tracker_);
  }
}  // namespace kagome::extensions
