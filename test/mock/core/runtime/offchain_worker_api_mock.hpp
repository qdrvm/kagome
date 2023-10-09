/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/offchain_worker_api.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class OffchainWorkerApiMock : public OffchainWorkerApi {
   public:
    MOCK_METHOD2(offchain_worker,
                 outcome::result<void>(const primitives::BlockHash &,
                                       const primitives::BlockHeader &));
  };

}  // namespace kagome::runtime
