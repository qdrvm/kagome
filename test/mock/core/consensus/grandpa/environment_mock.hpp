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
                onProposed,
                (RoundNumber round,
                 MembershipCounter set_id,
                 const SignedMessage &propose),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onPrevoted,
                (RoundNumber round,
                 MembershipCounter set_id,
                 const SignedMessage &prevote),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onPrecommitted,
                (RoundNumber round,
                 MembershipCounter set_id,
                 const SignedMessage &precommit),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onCommitted,
                (RoundNumber round,
                 const BlockInfo &vote,
                 const GrandpaJustification &justification),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onNeighborMessageSent,
                (RoundNumber round,
                 MembershipCounter set_id,
                 BlockNumber last_finalized),
                (override));

    MOCK_METHOD(void, doOnCompleted, (const CompleteHandler &), (override));

    MOCK_METHOD(void,
                onCompleted,
                (outcome::result<MovableRoundState> state),
                (override));

    MOCK_METHOD(outcome::result<void>,
                applyJustification,
                (const BlockInfo &block_info,
                 const primitives::Justification &justification),
                (override));

    MOCK_METHOD(outcome::result<void>,
                finalize,
                (MembershipCounter id,
                 const GrandpaJustification &justification),
                (override));

    MOCK_METHOD(outcome::result<GrandpaJustification>,
                getJustification,
                (const BlockHash &block_hash),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onCatchUpRequested,
                (const libp2p::peer::PeerId &peer_id,
                 MembershipCounter set_id,
                 RoundNumber round_number),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onCatchUpResponsed,
                (const libp2p::peer::PeerId &peer_id,
                 MembershipCounter set_id,
                 RoundNumber round_number,
                 std::vector<SignedPrevote> prevote_justification,
                 std::vector<SignedPrecommit> precommit_justification,
                 BlockInfo best_final_candidate),
                (override));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_MOCK_HPP
