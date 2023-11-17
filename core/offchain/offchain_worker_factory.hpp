/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "offchain/offchain_worker.hpp"

namespace kagome::offchain {

  class OffchainWorkerFactory {
   public:
    virtual ~OffchainWorkerFactory() = default;

    virtual std::shared_ptr<OffchainWorker> make() = 0;
  };

}  // namespace kagome::offchain
