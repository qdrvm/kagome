/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAINWORKERAPIMOCK
#define KAGOME_OFFCHAINWORKERAPIMOCK

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

#endif  // KAGOME_OFFCHAINWORKERAPIMOCK
