/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_MOCK_HPP

#include "mock/core/consensus/grandpa/chain_mock.hpp"
#include "consensus/grandpa/environment.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class EnvironmentMock : public Environment, public ChainMock {
   public:

    MOCK_METHOD3(onProposed,
                 outcome::result<void>(RoundNumber round,
                                       MembershipCounter set_id,
                                       const SignedMessage &propose));
    MOCK_METHOD3(onPrevoted,
                 outcome::result<void>(RoundNumber round,
                                       MembershipCounter set_id,
                                       const SignedMessage &prevote));
    MOCK_METHOD3(onPrecommitted,
                 outcome::result<void>(RoundNumber round,
                                       MembershipCounter set_id,
                                       const SignedMessage &precommit));
    MOCK_METHOD3(
        onCommitted,
        outcome::result<void>(RoundNumber round,
                              const BlockInfo &vote,
                              const GrandpaJustification &justification));

    MOCK_METHOD1(doOnCompleted, void(const CompleteHandler &));

    MOCK_METHOD1(onCompleted, void(outcome::result<MovableRoundState> state));

    MOCK_METHOD2(
        finalize,
        outcome::result<void>(const BlockHash &block,
                              const GrandpaJustification &justification));

    MOCK_METHOD1(
        getJustification,
        outcome::result<GrandpaJustification>(const BlockHash &block_hash));

    MOCK_METHOD3(onCatchUpRequested,
                 outcome::result<void>(const libp2p::peer::PeerId &peer_id,
                                       MembershipCounter set_id,
                                       RoundNumber round_number));

    MOCK_METHOD6(
        onCatchUpResponsed,
        outcome::result<void>(const libp2p::peer::PeerId &peer_id,
                              MembershipCounter set_id,
                              RoundNumber round_number,
                              GrandpaJustification prevote_justification,
                              GrandpaJustification precommit_justification,
                              BlockInfo best_final_candidate));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_MOCK_HPP
