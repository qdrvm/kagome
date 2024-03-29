/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/authority_manager.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  struct AuthorityManagerMock : public AuthorityManager {
    MOCK_METHOD(std::optional<std::shared_ptr<const AuthoritySet>>,
                authorities,
                (const primitives::BlockInfo &, IsBlockFinalized),
                (const, override));

    MOCK_METHOD(ScheduledParentResult,
                scheduledParent,
                (primitives::BlockInfo),
                (const, override));

    MOCK_METHOD(std::vector<primitives::BlockInfo>,
                possibleScheduled,
                (),
                (const, override));

    MOCK_METHOD(void,
                warp,
                (const primitives::BlockInfo &,
                 const primitives::BlockHeader &,
                 const AuthoritySet &),
                (override));
  };
}  // namespace kagome::consensus::grandpa
