/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "host_api/host_api_factory.hpp"

#include "crypto/ecdsa_provider.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/hasher.hpp"
#include "crypto/key_store.hpp"
#include "crypto/secp256k1_provider.hpp"
#include "crypto/sr25519_provider.hpp"
#include "host_api/impl/offchain_extension.hpp"

namespace kagome::offchain {
  class OffchainPersistentStorage;
  class OffchainWorkerPool;
}  // namespace kagome::offchain

namespace kagome::host_api {

  class HostApiFactoryImpl final : public HostApiFactory {
   public:
    ~HostApiFactoryImpl() override = default;

    HostApiFactoryImpl(
        const OffchainExtensionConfig &offchain_config,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
        std::shared_ptr<crypto::EcdsaProvider> ecdsa_provider,
        std::shared_ptr<crypto::Ed25519Provider> ed25519_provider,
        std::shared_ptr<crypto::Secp256k1Provider> secp256k1_provider,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<crypto::KeyStore> key_store,
        std::shared_ptr<offchain::OffchainPersistentStorage>
            offchain_persistent_storage,
        std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool);

    std::unique_ptr<HostApi> make(
        std::shared_ptr<const runtime::CoreApiFactory> core_factory,
        std::shared_ptr<const runtime::MemoryProvider> memory_provider,
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider)
        const override;

   private:
    OffchainExtensionConfig offchain_config_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
    std::shared_ptr<crypto::EcdsaProvider> ecdsa_provider_;
    std::shared_ptr<crypto::Ed25519Provider> ed25519_provider_;
    std::shared_ptr<crypto::Secp256k1Provider> secp256k1_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::KeyStore> key_store_;
    std::shared_ptr<offchain::OffchainPersistentStorage>
        offchain_persistent_storage_;
    std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool_;
  };

}  // namespace kagome::host_api
