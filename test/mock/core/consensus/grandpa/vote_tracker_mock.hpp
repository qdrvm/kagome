/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_VOTE_TRACKER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_VOTE_TRACKER_MOCK_HPP

#include "consensus/grandpa/vote_tracker.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class VoteTrackerMock : public VoteTracker {
   public:
    MOCK_METHOD(PushResult, push, (const SignedPrevote &, size_t), (override));

    MOCK_METHOD(PushResult,
                push,
                (const SignedPrecommit &, size_t),
                (override));

    MOCK_METHOD(std::vector<SignedPrevote>, getPrevotes, (), (const, override));

    MOCK_METHOD(std::vector<SignedPrecommit>,
                getPrecommits,
                (),
                (const, override));

    MOCK_METHOD(size_t, prevoteWeight, (), (const, override));

    MOCK_METHOD(size_t, precommitWeight, (), (const, override));

    MOCK_METHOD(Justification,
                getJustification,
                (const BlockInfo &),
                (const, override));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_VOTE_TRACKER_MOCK_HPP
