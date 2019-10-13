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
    MOCK_METHOD2(push, PushResult(const SignedPrevote &, size_t));
    MOCK_METHOD2(push, PushResult(const SignedPrecommit &, size_t));

    MOCK_CONST_METHOD0(getPrevotes, std::vector<SignedPrevote>());
    MOCK_CONST_METHOD0(getPrecommits, std::vector<SignedPrecommit>());

    MOCK_CONST_METHOD0(prevoteWeight, size_t());
    MOCK_CONST_METHOD0(precommitWeight, size_t());

    MOCK_CONST_METHOD1(getJustification, Justification(const BlockInfo &));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_VOTE_TRACKER_MOCK_HPP
