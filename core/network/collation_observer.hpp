/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>

#include "network/types/collator_messages_vstaging.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to collation protocol
   */
  class CollationObserver {
   public:
    virtual ~CollationObserver() = default;

    /// Handle incoming collation message.
    virtual void onIncomingMessage(
        const libp2p::peer::PeerId &peer_id,
        network::VersionedCollatorProtocolMessage &&msg) = 0;
  };

}  // namespace kagome::network
