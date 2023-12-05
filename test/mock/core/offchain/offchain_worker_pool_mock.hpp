/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "offchain/offchain_worker_pool.hpp"

namespace kagome::offchain {

  class OffchainWorkerPoolMock : public OffchainWorkerPool {
   public:
    MOCK_METHOD(void,
                addWorker,
                (std::shared_ptr<OffchainWorker> ofw),
                (override));

    MOCK_METHOD(std::optional<std::shared_ptr<OffchainWorker>>,
                getWorker,
                (),
                (const, override));

    MOCK_METHOD(bool, removeWorker, (), (override));
  };

}  // namespace kagome::offchain
