/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>

#include "network/types/collator_messages.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to collation protocol
   */
  class CollationObserver {
   public:
    virtual ~CollationObserver() = default;

    /// Handle incoming collation stream.
    virtual void onIncomingCollationStream(
        const libp2p::peer::PeerId &peer_id) = 0;

    /// Handle incoming collation message.
    virtual void onIncomingMessage(
        const libp2p::peer::PeerId &peer_id,
        CollationProtocolMessage &&collation_message) = 0;
  };
}  // namespace kagome::network
