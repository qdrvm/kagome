/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROPOSER_MOCK_HPP
#define KAGOME_PROPOSER_MOCK_HPP

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

#endif  // KAGOME_PROPOSER_MOCK_HPP
