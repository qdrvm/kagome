/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_GRANDPATRANSMITTER
#define KAGOME_NETWORK_GRANDPATRANSMITTER

#include <libp2p/peer/peer_id.hpp>

#include "network/types/grandpa_message.hpp"

namespace kagome::network {

  /**
   * Send/broadcast grandpa messages
   */
  class GrandpaTransmitter {
   public:
    virtual ~GrandpaTransmitter() = default;

    virtual void sendNeighborMessage(GrandpaNeighborMessage &&message) = 0;

    virtual void sendVoteMessage(const libp2p::peer::PeerId &peer_id,
                                 GrandpaVote &&message) = 0;

    virtual void sendVoteMessage(GrandpaVote &&message) = 0;

    virtual void sendCommitMessage(const libp2p::peer::PeerId &peer_id,
                                   FullCommitMessage &&message) = 0;

    virtual void sendCommitMessage(FullCommitMessage &&message) = 0;

    virtual void sendCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                                    CatchUpRequest &&message) = 0;

    virtual void sendCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                                     CatchUpResponse &&message) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_GRANDPATRANSMITTER
