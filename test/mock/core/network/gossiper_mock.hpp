/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_GOSSIPER_MOCK_HPP
#define KAGOME_NETWORK_GOSSIPER_MOCK_HPP

#include "network/gossiper.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class GossiperMock final : public Gossiper {
   public:
    // From BabeGossiper
    MOCK_METHOD1(blockAnnounce, void(const BlockAnnounce &announce));

    // From grandpa::Gossiper
    MOCK_METHOD1(vote, void(const GrandpaVoteMessage &vote_message));
    MOCK_METHOD1(finalize, void(const GrandpaPreCommit &message));

    MOCK_METHOD2(catchUpRequest,
                 void(const libp2p::peer::PeerId &peer_id,
                      const CatchUpRequest &catch_up_request));

    MOCK_METHOD2(catchUpResponse,
                 void(const libp2p::peer::PeerId &peer_id,
                      const CatchUpResponse &catch_up_response));

    // From ExtrinsicGossiper
    MOCK_METHOD1(propagateTransactions,
                 void(gsl::span<const primitives::Transaction> txs));

    MOCK_METHOD1(storeSelfPeerInfo,
                 void(const libp2p::peer::PeerInfo &self_info));
  };

}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_MOCK_HPP
