/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_WORKER_POOL_IMPL_HPP
#define KAGOME_OFFCHAIN_WORKER_POOL_IMPL_HPP

#include <thread>

#include "offchain/offchain_worker_pool.hpp"

#include "log/logger.hpp"

namespace kagome::offchain {
  using soralog::util::ThreadNumber;

  class OffchainWorkerPoolImpl final : public OffchainWorkerPool {
   public:
    OffchainWorkerPoolImpl();

    void addWorker(std::shared_ptr<OffchainWorker> ofw) override;

    std::optional<std::shared_ptr<OffchainWorker>> getWorker() override;

    bool removeWorker() override;

   private:
    std::mutex mut_;

    log::Logger log_;

    std::unordered_map<ThreadNumber, std::shared_ptr<OffchainWorker>>
        offchain_workers_ = {};
  };

}  // namespace kagome::offchain

#endif /* KAGOME_OFFCHAIN_WORKER_POOL_IMPL_HPP */
