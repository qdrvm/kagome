/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
        std::optional<std::shared_ptr<GrandpaContext>> &&existed_context,
        const libp2p::peer::PeerId &peer_id,
        std::optional<network::PeerStateCompact> &&info_opt,
        const VoteMessage &msg) = 0;

    /**
     * Handler of grandpa finalization messages
     * @param peer_id finalization sender
     * @param f finalization message
     */
    virtual void onCommitMessage(
        std::optional<std::shared_ptr<GrandpaContext>> &&existed_context,
        const libp2p::peer::PeerId &peer_id,
        const network::FullCommitMessage &msg) = 0;
  };

}  // namespace kagome::consensus::grandpa
