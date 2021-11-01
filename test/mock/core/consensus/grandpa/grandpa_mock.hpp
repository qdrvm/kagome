/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_GRANDPA_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_GRANDPA_MOCK_HPP

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
                 const network::GrandpaNeighborMessage &msg),
                (override));

    MOCK_METHOD(void,
                onVoteMessage,
                (const PeerId &peer_id, const VoteMessage &),
                (override));

    MOCK_METHOD(void,
                onFinalize,
                (const PeerId &peer_id, const network::FullCommitMessage &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                applyJustification,
                (const primitives::BlockInfo &block_info,
                 const GrandpaJustification &justification),
                (override));

    MOCK_METHOD(void,
                onCatchUpRequest,
                (const PeerId &peer_id, const CatchUpRequest &),
                (override));

    MOCK_METHOD(void,
                onCatchUpResponse,
                (const PeerId &peer_id, const CatchUpResponse &),
                (override));

    MOCK_METHOD(void, executeNextRound, (), (override));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_GRANDPA_MOCK_HPP
