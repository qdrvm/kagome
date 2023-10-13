/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "blockchain/impl/justification_storage_policy.hpp"

namespace kagome::blockchain {

  class JustificationStoragePolicyMock : public JustificationStoragePolicy {
   public:
    MOCK_METHOD(outcome::result<bool>,
                shouldStoreFor,
                (const primitives::BlockHeader &header,
                 primitives::BlockNumber last_finalized_number),
                (const));

   private:
  };

}  // namespace kagome::blockchain
