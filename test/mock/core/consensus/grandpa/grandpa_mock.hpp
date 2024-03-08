/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/grandpa.hpp"
#include "consensus/grandpa/grandpa_observer.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class GrandpaMock : public Grandpa, public GrandpaObserver {
    using PeerId = libp2p::peer::PeerId;
    using CatchUpRequest = network::CatchUpRequest;
    using CatchUpResponse = network::CatchUpResponse;

   public:
    MOCK_METHOD(void,
                onNeighborMessage,
                (const libp2p::peer::PeerId &peer_id,
                 std::optional<network::PeerStateCompact> &&,
                 network::GrandpaNeighborMessage &&msg),
                (override));

    MOCK_METHOD(void,
                onVoteMessage,
                (const PeerId &,
                 std::optional<network::PeerStateCompact> &&,
                 VoteMessage &&),
                (override));

    MOCK_METHOD(void,
                onCommitMessage,
                (const PeerId &, network::FullCommitMessage &&),
                (override));

    MOCK_METHOD(outcome::result<void>,
                verifyJustification,
                (const GrandpaJustification &, const AuthoritySet &),
                (override));

    MOCK_METHOD(void,
                applyJustification,
                (const GrandpaJustification &justification,
                 ApplyJustificationCb &&),
                (override));

    MOCK_METHOD(void, reload, (), (override));

    MOCK_METHOD(void,
                onCatchUpRequest,
                (const PeerId &peer_id,
                 std::optional<network::PeerStateCompact> &&,
                 CatchUpRequest &&),
                (override));

    MOCK_METHOD(void,
                onCatchUpResponse,
                (const PeerId &, CatchUpResponse &&),
                (override));

    MOCK_METHOD(void,
                tryExecuteNextRound,
                (const std::shared_ptr<VotingRound> &),
                (override));

    MOCK_METHOD(void, updateNextRound, (RoundNumber round_number), (override));
  };

}  // namespace kagome::consensus::grandpa
