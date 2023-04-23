/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_MOCK_HPP

#include "consensus/grandpa/environment.hpp"
#include "mock/core/consensus/grandpa/chain_mock.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class EnvironmentMock : public Environment, public ChainMock {
   public:
    MOCK_METHOD(void,
                setJustificationObserver,
                (std::weak_ptr<JustificationObserver> justification_observer),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onVoted,
                (RoundNumber round,
                 VoterSetId set_id,
                 const SignedMessage &propose),
                (override));

    MOCK_METHOD(void,
                sendState,
                (const libp2p::peer::PeerId &peer_id,
                 const MovableRoundState &state,
                 VoterSetId set_id),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onCommitted,
                (RoundNumber round,
                 VoterSetId set_id,
                 const BlockInfo &vote,
                 const GrandpaJustification &justification),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onNeighborMessageSent,
                (RoundNumber round,
                 VoterSetId set_id,
                 BlockNumber last_finalized),
                (override));

    void applyJustification (const BlockInfo &block_info,
                 const primitives::Justification &justification,
                 Environment::ApplyJustificationCb &&cb) override {
                  cb(outcome::success());
                 }

    MOCK_METHOD(outcome::result<void>,
                finalize,
                (VoterSetId id, const GrandpaJustification &justification),
                (override));

    MOCK_METHOD(outcome::result<GrandpaJustification>,
                getJustification,
                (const BlockHash &block_hash),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onCatchUpRequested,
                (const libp2p::peer::PeerId &peer_id,
                 VoterSetId set_id,
                 RoundNumber round_number),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onCatchUpRespond,
                (const libp2p::peer::PeerId &peer_id,
                 VoterSetId set_id,
                 RoundNumber round_number,
                 std::vector<SignedPrevote> prevote_justification,
                 std::vector<SignedPrecommit> precommit_justification,
                 BlockInfo best_final_candidate),
                (override));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_MOCK_HPP
