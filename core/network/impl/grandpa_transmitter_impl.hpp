/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/grandpa_transmitter.hpp"

namespace kagome::network {
  class Router;

  class GrandpaTransmitterImpl final : public GrandpaTransmitter {
   public:
    GrandpaTransmitterImpl(std::shared_ptr<Router> router);

    void sendNeighborMessage(GrandpaNeighborMessage &&message) override;

    void sendVoteMessage(const libp2p::peer::PeerId &peer_id,
                         GrandpaVote &&message) override;

    void sendVoteMessage(GrandpaVote &&message) override;

    void sendCommitMessage(const libp2p::peer::PeerId &peer_id,
                           FullCommitMessage &&message) override;

    void sendCommitMessage(FullCommitMessage &&message) override;

    void sendCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                            CatchUpRequest &&message) override;

    void sendCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                             CatchUpResponse &&message) override;

   private:
    std::shared_ptr<Router> router_;
  };

}  // namespace kagome::network
