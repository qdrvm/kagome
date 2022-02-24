/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_WORKER_POOL_MOCK_HPP
#define KAGOME_OFFCHAIN_WORKER_POOL_MOCK_HPP

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
                (override));

    MOCK_METHOD(bool, removeWorker, (), (override));
  };

}  // namespace kagome::offchain

#endif /* KAGOME_OFFCHAIN_WORKER_POOL_MOCK_HPP */
