/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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

    virtual void vote(network::GrandpaVote &&message) = 0;
    virtual void finalize(network::FullCommitMessage &&message) = 0;
    virtual void catchUpRequest(const libp2p::peer::PeerId &peer_id,
                                network::CatchUpRequest &&message) = 0;
    virtual void catchUpResponse(const libp2p::peer::PeerId &peer_id,
                                 network::CatchUpResponse &&message) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_GRANDPATRANSMITTER
