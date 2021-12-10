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
    MOCK_METHOD(RoundNumber, roundNumber, (), (const, override));

    MOCK_METHOD(MembershipCounter, voterSetId, (), (const, override));

    MOCK_METHOD(bool, completable, (), (const, override));

    MOCK_METHOD(bool, finalizable, (), (const, override));

    MOCK_METHOD(BlockInfo, lastFinalizedBlock, (), (const, override));

    MOCK_METHOD(BlockInfo, bestPrevoteCandidate, (), (override));

    MOCK_METHOD(BlockInfo, bestPrecommitCandidate, (), (override));

    MOCK_METHOD(BlockInfo, bestFinalCandidate, (), (override));

    MOCK_METHOD(std::optional<BlockInfo>,
                finalizedBlock,
                (),
                (const, override));

    MOCK_METHOD(MovableRoundState, state, (), (const, override));

    MOCK_METHOD(void, play, (), (override));

    MOCK_METHOD(void, end, (), (override));

    MOCK_METHOD(void, doProposal, (), (override));

    MOCK_METHOD(void, doPrevote, (), (override));

    MOCK_METHOD(void, doPrecommit, (), (override));

    MOCK_METHOD(void, doFinalize, (), (override));

    MOCK_METHOD(void, doCommit, (), (override));

    MOCK_METHOD(void, doCatchUpRequest, (const libp2p::peer::PeerId &), ());

    MOCK_METHOD(void,
                doCatchUpResponse,
                (const libp2p::peer::PeerId &),
                (override));

    MOCK_METHOD(void,
                onProposal,
                (const SignedMessage &, Propagation),
                (override));

    MOCK_METHOD(bool,
                onPrevote,
                (const SignedMessage &, Propagation),
                (override));

    MOCK_METHOD(bool,
                onPrecommit,
                (const SignedMessage &, Propagation),
                (override));

    MOCK_METHOD(void, update, (bool, bool), (override));

    MOCK_METHOD(outcome::result<void>,
                applyJustification,
                (const BlockInfo &, const GrandpaJustification &),
                (override));

    MOCK_METHOD(void, attemptToFinalizeRound, (), (override));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_VOTINGROUNDMOCK
