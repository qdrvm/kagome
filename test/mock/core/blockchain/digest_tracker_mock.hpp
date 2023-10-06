/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/digest_tracker.hpp"

#include <gmock/gmock.h>

namespace kagome::blockchain {

  class DigestTrackerMock : public DigestTracker {
   public:
    MOCK_METHOD(outcome::result<void>,
                onDigest,
                (const primitives::BlockContext &, const primitives::Digest &),
                (override));

    MOCK_METHOD(void, cancel, (const primitives::BlockInfo &), (override));
  };
}  // namespace kagome::blockchain
