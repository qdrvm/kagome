/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_VOTINGROUNDMOCK
#define KAGOME_CONSENSUS_GRANDPA_VOTINGROUNDMOCK

#include "consensus/grandpa/voting_round.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class VotingRoundMock : public VotingRound {
   public:
    MOCK_CONST_METHOD0(roundNumber, RoundNumber());
    MOCK_CONST_METHOD0(voterSetId, MembershipCounter());
    MOCK_CONST_METHOD0(completable, bool());
    MOCK_CONST_METHOD0(finalizable, bool());
    MOCK_CONST_METHOD0(lastFinalizedBlock, BlockInfo());
    MOCK_METHOD0(bestPrevoteCandidate, BlockInfo());
    MOCK_METHOD0(bestPrecommitCandidate, BlockInfo());
    MOCK_METHOD0(bestFinalCandidate, BlockInfo());
    MOCK_CONST_METHOD0(finalizedBlock, boost::optional<BlockInfo>());
    MOCK_CONST_METHOD0(state, MovableRoundState());
    MOCK_METHOD0(play, void());
    MOCK_METHOD0(end, void());
    MOCK_METHOD0(doProposal, void());
    MOCK_METHOD0(doPrevote, void());
    MOCK_METHOD0(doPrecommit, void());
    MOCK_METHOD0(doFinalize, void());
    MOCK_METHOD1(doCatchUpRequest, void(const libp2p::peer::PeerId &));
    MOCK_METHOD1(doCatchUpResponse, void(const libp2p::peer::PeerId &));
    MOCK_METHOD2(onProposal, void(const SignedMessage &, bool));
    MOCK_METHOD2(onPrevote, bool(const SignedMessage &, bool));
    MOCK_METHOD2(onPrecommit, bool(const SignedMessage &, bool));
    MOCK_METHOD2(update, void(bool, bool));
    MOCK_METHOD2(applyJustification,
                 outcome::result<void>(const BlockInfo &,
                                       const GrandpaJustification &));
    MOCK_METHOD0(attemptToFinalizeRound, void());
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_VOTINGROUNDMOCK
