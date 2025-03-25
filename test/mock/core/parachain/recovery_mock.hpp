/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "parachain/availability/recovery/recovery.hpp"

namespace kagome::parachain {

  class RecoveryMock : public Recovery {
   public:
    ~RecoveryMock() override = default;

    MOCK_METHOD(void, 
                recover, 
                (const HashedCandidateReceipt &, 
                 SessionIndex, 
                 std::optional<GroupIndex>,
                 std::optional<CoreIndex>,
                 Cb), 
                (override));
                
    MOCK_METHOD(void, 
                remove, 
                (const CandidateHash &), 
                (override));
  };

}  // namespace kagome::parachain 