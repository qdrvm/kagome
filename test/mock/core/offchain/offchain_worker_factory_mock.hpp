/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "offchain/offchain_worker_factory.hpp"

namespace kagome::offchain {

  class OffchainWorkerFactoryMock : public OffchainWorkerFactory {
   public:
    MOCK_METHOD2(
        make,
        std::shared_ptr<OffchainWorker>(std::shared_ptr<runtime::Executor>,
                                        const primitives::BlockHeader &));
  };

}  // namespace kagome::offchain
