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
    MOCK_METHOD(std::shared_ptr<OffchainWorker>, make, (), (override));
  };

}  // namespace kagome::offchain
