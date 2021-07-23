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
    MOCK_METHOD2(onNeighborMessage,
                 void(const libp2p::peer::PeerId &peer_id,
                      const network::GrandpaNeighborMessage &msg));

    MOCK_METHOD2(onVoteMessage,
                 void(const PeerId &peer_id, const VoteMessage &));
    MOCK_METHOD2(onFinalize,
                 void(const PeerId &peer_id,
                      const network::FullCommitMessage &));

    MOCK_METHOD2(
        applyJustification,
        outcome::result<void>(const primitives::BlockInfo &block_info,
                              const GrandpaJustification &justification));

    MOCK_METHOD2(onCatchUpRequest,
                 void(const PeerId &peer_id, const CatchUpRequest &));
    MOCK_METHOD2(onCatchUpResponse,
                 void(const PeerId &peer_id, const CatchUpResponse &));

    MOCK_METHOD0(executeNextRound, void());
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_GRANDPA_MOCK_HPP
