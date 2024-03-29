/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/grandpa_context.hpp"
#include "network/peer_manager.hpp"

namespace libp2p::peer {
  class PeerId;
}

namespace kagome::consensus::grandpa {
  struct VoteMessage;
}

namespace kagome::network {
  struct FullCommitMessage;
}

namespace kagome::consensus::grandpa {

  /**
   * @class RoundObserver
   * @brief observes incoming messages. Abstraction of a network.
   */
  struct RoundObserver {
    virtual ~RoundObserver() = default;

    /**
     * Handler of grandpa vote messages
     * @param peer_id vote owner
     * @param msg vote message
     */
    virtual void onVoteMessage(
        const libp2p::peer::PeerId &peer_id,
        std::optional<network::PeerStateCompact> &&info_opt,
        VoteMessage &&msg) = 0;

    /**
     * Handler of grandpa finalization messages
     * @param peer_id finalization sender
     * @param f finalization message
     */
    virtual void onCommitMessage(const libp2p::peer::PeerId &peer_id,
                                 network::FullCommitMessage &&msg) = 0;
  };

}  // namespace kagome::consensus::grandpa
