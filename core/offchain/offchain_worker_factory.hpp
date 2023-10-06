/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINWORKERFACTORY
#define KAGOME_OFFCHAIN_OFFCHAINWORKERFACTORY

#include "offchain/offchain_worker.hpp"

#include "primitives/block_header.hpp"
#include "runtime/executor.hpp"

namespace kagome::offchain {

  class OffchainWorkerFactory {
   public:
    virtual ~OffchainWorkerFactory() = default;

    virtual std::shared_ptr<OffchainWorker> make(
        std::shared_ptr<runtime::Executor> executor,
        const primitives::BlockHeader &header) = 0;
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINWORKERFACTORY
