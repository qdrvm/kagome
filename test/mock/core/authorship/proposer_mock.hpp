/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "authorship/proposer.hpp"

#include <gmock/gmock.h>

namespace kagome::authorship {
  class ProposerMock : public Proposer {
   public:
    MOCK_METHOD(outcome::result<primitives::Block>,
                propose,
                (const primitives::BlockInfo &,
                 std::optional<Clock::TimePoint>,
                 const primitives::InherentData &,
                 const primitives::Digest &,
                 TrieChangesTrackerOpt),
                (override));
  };
}  // namespace kagome::authorship
