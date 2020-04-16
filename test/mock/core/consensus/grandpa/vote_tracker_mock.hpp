/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
