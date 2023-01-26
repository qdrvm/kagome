/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINWORKERFACTORYIMPL
#define KAGOME_OFFCHAIN_OFFCHAINWORKERFACTORYIMPL

#include "offchain/offchain_worker_factory.hpp"

#include <libp2p/host/host.hpp>

#include "crypto/random_generator.hpp"
#include "network/types/own_peer_info.hpp"
#include "offchain/offchain_persistent_storage.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::api {
  class AuthorApi;
}
namespace kagome::application {
  class AppConfiguration;
}
namespace kagome::crypto {
  class Hasher;
}
namespace kagome::runtime {
  class Executor;
}

namespace kagome::offchain {
  class OffchainWorkerPool;
  class OffchainWorkerFactoryImpl final : public OffchainWorkerFactory {
   public:
    OffchainWorkerFactoryImpl(
        const application::AppConfiguration &app_config,
        std::shared_ptr<clock::SystemClock> clock,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<storage::SpacedStorage> storage,
        std::shared_ptr<crypto::CSPRNG> random_generator,
        std::shared_ptr<api::AuthorApi> author_api,
        const network::OwnPeerInfo &current_peer_info,
        std::shared_ptr<offchain::OffchainPersistentStorage> persistent_storage,
        std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool);

    std::shared_ptr<OffchainWorker> make(
        std::shared_ptr<runtime::Executor> executor,
        const primitives::BlockHeader &header) override;

   private:
    const application::AppConfiguration &app_config_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<storage::SpacedStorage> storage_;
    std::shared_ptr<crypto::CSPRNG> random_generator_;
    std::shared_ptr<api::AuthorApi> author_api_;
    const network::OwnPeerInfo &current_peer_info_;
    std::shared_ptr<offchain::OffchainPersistentStorage> persistent_storage_;
    std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool_;
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINWORKERFACTORYIMPL
