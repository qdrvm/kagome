/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>

#include "network/types/grandpa_message.hpp"
#include "utils/box.hpp"

namespace kagome::consensus::grandpa {

  /**
   * @class NeighborObserver
   * @brief observes incoming neighbor messages. Abstraction of a network.
   */
  struct NeighborObserver {
    virtual ~NeighborObserver() = default;

    /**
     * Handler of grandpa neighbor messages
     * @param msg neighbor messages
     */
    virtual void onNeighborMessage(
        const libp2p::peer::PeerId &peer_id,
        std::optional<network::PeerStateCompact> &&info_opt,
        network::GrandpaNeighborMessage &&msg) = 0;
  };

}  // namespace kagome::consensus::grandpa
