/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/host_api_factory_impl.hpp"

#include "host_api/impl/host_api_impl.hpp"

namespace kagome::host_api {

  HostApiFactoryImpl::HostApiFactoryImpl(
      std::shared_ptr<storage::changes_trie::ChangesTracker> tracker,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
      std::shared_ptr<crypto::Ed25519Provider> ed25519_provider,
      std::shared_ptr<crypto::Secp256k1Provider> secp256k1_provider,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<crypto::CryptoStore> crypto_store,
      std::shared_ptr<crypto::Bip39Provider> bip39_provider)
      : changes_tracker_{std::move(tracker)},
        sr25519_provider_(std::move(sr25519_provider)),
        ed25519_provider_(std::move(ed25519_provider)),
        secp256k1_provider_(std::move(secp256k1_provider)),
        hasher_(std::move(hasher)),
        crypto_store_(std::move(crypto_store)),
        bip39_provider_(std::move(bip39_provider)) {
    BOOST_ASSERT(changes_tracker_ != nullptr);
    BOOST_ASSERT(sr25519_provider_ != nullptr);
    BOOST_ASSERT(ed25519_provider_ != nullptr);
    BOOST_ASSERT(secp256k1_provider_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(crypto_store_ != nullptr);
    BOOST_ASSERT(bip39_provider_ != nullptr);
  }

  std::unique_ptr<HostApi> HostApiFactoryImpl::make(
      std::shared_ptr<const runtime::CoreApiFactory> core_provider,
      std::shared_ptr<const runtime::MemoryProvider> memory_provider,
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider) const {
    return std::make_unique<HostApiImpl>(memory_provider,
                                         core_provider,
                                         storage_provider,
                                         changes_tracker_,
                                         sr25519_provider_,
                                         ed25519_provider_,
                                         secp256k1_provider_,
                                         hasher_,
                                         crypto_store_,
                                         bip39_provider_);
  }
}  // namespace kagome::host_api
