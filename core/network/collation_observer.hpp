/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COLLATION_OBSERVER_HPP
#define KAGOME_COLLATION_OBSERVER_HPP

#include <libp2p/peer/peer_id.hpp>

#include "network/types/collator_messages.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to collation protocol
   */
  struct CollationObserver {
    virtual ~CollationObserver() = default;

    /// Handle incoming collation stream.
    virtual void onIncomingCollationStream(
        libp2p::peer::PeerId const &peer_id) = 0;

    /// Handle incoming collation message.
    virtual void onIncomingMessage(
        libp2p::peer::PeerId const &peer_id,
        CollationProtocolMessage &&collation_message) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_COLLATION_OBSERVER_HPP
