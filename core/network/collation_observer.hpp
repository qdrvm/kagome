/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COLLATION_OBSERVER_HPP
#define KAGOME_COLLATION_OBSERVER_HPP

#include <libp2p/peer/peer_id.hpp>

#include <libp2p/peer/peer_id.hpp>
#include "consensus/grandpa/common.hpp"
#include "network/types/collator_messages.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to collation protocol
   */
  struct CollationObserver {
    virtual ~CollationObserver() = default;

    /**
     * Triggered when a Peer makes advertisement
     * @param peer_id id of the peer
     * @param para_hash hash of the parachain block
     */
    virtual void onAdvertise(libp2p::peer::PeerId const &peer_id,
                             primitives::BlockHash para_hash) = 0;

    /**
     * Triggered when a Peer declares as a collator
     */
    virtual void onDeclare(
        libp2p::peer::PeerId const &peer_id,  /// PeerId of the peer.
        CollatorPublicKey pubkey,             /// Public key of the collator.
        ParachainId para_id,                  /// Parachain Id.
        Signature signature                   /// Signature of the collator
                                              /// using the PeerId of the
                                              /// collators node.
        ) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_COLLATION_OBSERVER_HPP
