/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "offchain/offchain_worker.hpp"

namespace kagome::offchain {
  class OffchainWorkerPool {
   public:
    virtual ~OffchainWorkerPool() = default;

    virtual void addWorker(std::shared_ptr<OffchainWorker> ofw) = 0;

    virtual std::optional<std::shared_ptr<OffchainWorker>> getWorker()
        const = 0;

    virtual bool removeWorker() = 0;
  };
}  // namespace kagome::offchain
