/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_WORKER_POOL_IMPL_HPP
#define KAGOME_OFFCHAIN_WORKER_POOL_IMPL_HPP

#include <thread>

#include "offchain/offchain_worker_pool.hpp"

#include "log/logger.hpp"
#include "utils/safe_object.hpp"

namespace kagome::offchain {
  using kagome::primitives::ThreadNumber;

  class OffchainWorkerPoolImpl final : public OffchainWorkerPool {
   public:
    OffchainWorkerPoolImpl();

    void addWorker(std::shared_ptr<OffchainWorker> ofw) override;

    std::optional<std::shared_ptr<OffchainWorker>> getWorker() const override;

    bool removeWorker() override;

   private:
    log::Logger log_;
    SafeObject<
        std::unordered_map<ThreadNumber, std::shared_ptr<OffchainWorker>>>
        offchain_workers_;
  };

}  // namespace kagome::offchain

#endif /* KAGOME_OFFCHAIN_WORKER_POOL_IMPL_HPP */
