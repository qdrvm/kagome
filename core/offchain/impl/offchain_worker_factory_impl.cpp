/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/offchain_worker_factory_impl.hpp"

#include "offchain/impl/offchain_worker_impl.hpp"
#include "offchain/offchain_worker_pool.hpp"

namespace kagome::offchain {

  OffchainWorkerFactoryImpl::OffchainWorkerFactoryImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<clock::SystemClock> clock,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<crypto::CSPRNG> random_generator,
      std::shared_ptr<api::AuthorApi> author_api,
      const network::OwnPeerInfo &current_peer_info,
      std::shared_ptr<offchain::OffchainPersistentStorage> persistent_storage,
      std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool)
      : app_config_(app_config),
        clock_(std::move(clock)),
        hasher_(std::move(hasher)),
        storage_(std::move(storage)),
        random_generator_(std::move(random_generator)),
        author_api_(std::move(author_api)),
        current_peer_info_(current_peer_info),
        persistent_storage_(std::move(persistent_storage)),
        offchain_worker_pool_(std::move(offchain_worker_pool)) {
    BOOST_ASSERT(clock_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(storage_);
    BOOST_ASSERT(random_generator_);
    BOOST_ASSERT(author_api_);
    BOOST_ASSERT(persistent_storage_);
    BOOST_ASSERT(offchain_worker_pool_);
  }

  std::shared_ptr<OffchainWorker> OffchainWorkerFactoryImpl::make(
      std::shared_ptr<runtime::Executor> executor,
      const primitives::BlockHeader &header) {
    return std::make_shared<OffchainWorkerImpl>(app_config_,
                                                clock_,
                                                hasher_,
                                                storage_,
                                                random_generator_,
                                                author_api_,
                                                current_peer_info_,
                                                persistent_storage_,
                                                executor,
                                                header,
                                                offchain_worker_pool_);
  }

}  // namespace kagome::offchain
