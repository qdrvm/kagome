/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_ANNOUNCE_OBSERVER_HPP
#define KAGOME_BLOCK_ANNOUNCE_OBSERVER_HPP

#include <libp2p/peer/peer_id.hpp>

#include "network/types/block_announce.hpp"
#include "network/types/block_announce_handshake.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to block announce protocol
   */
  struct BlockAnnounceObserver {
    virtual ~BlockAnnounceObserver() = default;

    /**
     * Triggered when a Status arrives (as handshake of block announce protocol)
     * @param status - remote status
     */
    virtual void onBlockAnnounceHandshake(
        const libp2p::peer::PeerId &peer_id,
        const BlockAnnounceHandshake &block_announce_handshake) = 0;

    /**
     * Triggered when a BlockAnnounce message arrives
     * @param announce - arrived message
     */
    virtual void onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                                 const BlockAnnounce &announce) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_BLOCK_ANNOUNCE_OBSERVER_HPP
