/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COLLATION_OBSERVER_HPP
#define KAGOME_COLLATION_OBSERVER_HPP

#include <libp2p/peer/peer_id.hpp>

namespace kagome::network {
  /**
   * Reacts to messages, related to collation protocol
   */
  struct CollationObserver {
    virtual ~CollationObserver() = default;

    /**
     * Triggered when a Peer requests collation
     * @param peer_id is the identificator of peer
     */
    virtual void onRequestCollation(const libp2p::peer::PeerId &peer_id) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_COLLATION_OBSERVER_HPP
