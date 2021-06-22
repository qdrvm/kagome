/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_OBSERVER_HPP
#define KAGOME_BABE_OBSERVER_HPP

#include <libp2p/peer/peer_id.hpp>

#include "network/types/block_announce.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to BABE
   */
  struct BabeObserver {
    virtual ~BabeObserver() = default;

    /**
     * Triggered when a BlockAnnounce message arrives
     * @param announce - arrived message
     */
    virtual void onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                                 const BlockAnnounce &announce) = 0;
    virtual void onSync() = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_BABE_OBSERVER_HPP
