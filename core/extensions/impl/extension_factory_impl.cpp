/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/extension_factory_impl.hpp"

#include "extensions/impl/extension_impl.hpp"

namespace kagome::extensions {

  ExtensionFactoryImpl::ExtensionFactoryImpl(
      std::shared_ptr<storage::changes_trie::ChangesTracker> tracker,
      std::shared_ptr<crypto::SR25519Provider> sr25519_provider,
      std::shared_ptr<crypto::ED25519Provider> ed25519_provider,
      std::shared_ptr<crypto::Secp256k1Provider> secp256k1_provider,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<crypto::CryptoStore> crypto_store,
      std::shared_ptr<crypto::Bip39Provider> bip39_provider,
      MiscExtension::CoreFactoryMethod core_factory_method)
      : changes_tracker_{std::move(tracker)},
        sr25519_provider_(std::move(sr25519_provider)),
        ed25519_provider_(std::move(ed25519_provider)),
        secp256k1_provider_(std::move(secp256k1_provider)),
        hasher_(std::move(hasher)),
        crypto_store_(std::move(crypto_store)),
        bip39_provider_(std::move(bip39_provider)),
        core_factory_method_{std::move(core_factory_method)}{
    BOOST_ASSERT(changes_tracker_ != nullptr);
    BOOST_ASSERT(sr25519_provider_ != nullptr);
    BOOST_ASSERT(ed25519_provider_ != nullptr);
    BOOST_ASSERT(secp256k1_provider_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(crypto_store_ != nullptr);
    BOOST_ASSERT(bip39_provider_ != nullptr);
    BOOST_ASSERT(core_factory_method_ != nullptr);
  }

  std::unique_ptr<Extension> ExtensionFactoryImpl::createExtension(
      std::shared_ptr<runtime::WasmMemory> memory,
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider) const {
    return std::make_unique<ExtensionImpl>(memory,
                                           storage_provider,
                                           changes_tracker_,
                                           sr25519_provider_,
                                           ed25519_provider_,
                                           secp256k1_provider_,
                                           hasher_,
                                           crypto_store_,
                                           bip39_provider_,
                                           core_factory_method_);
  }
}  // namespace kagome::extensions
