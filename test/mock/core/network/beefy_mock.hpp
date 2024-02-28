/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/beefy.hpp"

#include <gmock/gmock.h>

namespace kagome::network {
  struct BeefyMock : public Beefy {
    MOCK_METHOD(primitives::BlockNumber, finalized, (), (const, override));

    MOCK_METHOD(
        outcome::result<std::optional<consensus::beefy::BeefyJustification>>,
        getJustification,
        (primitives::BlockNumber),
        (const, override));

    MOCK_METHOD(void,
                onJustification,
                (const primitives::BlockHash &, primitives::Justification),
                (override));

    MOCK_METHOD(void,
                onMessage,
                (consensus::beefy::BeefyGossipMessage),
                (override));
  };
}  // namespace kagome::network

// Trick to make mock-method compilable
namespace kagome::consensus::beefy {
  std::ostream &operator<<(std::ostream &s, const VoteMessage &) {
    return s;
  }
  std::ostream &operator<<(std::ostream &s, const SignedCommitment &) {
    return s;
  }
}  // namespace kagome::consensus::beefy
