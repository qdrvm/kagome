/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_VALIDATION_OBSERVER_HPP
#define KAGOME_VALIDATION_OBSERVER_HPP

#include <libp2p/peer/peer_id.hpp>

#include "network/types/collator_messages_vstaging.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to collation protocol
   */
  class ValidationObserver {
   public:
    virtual ~ValidationObserver() = default;

    /// Handle incoming validation stream.
    virtual void onIncomingValidationStream(
        const libp2p::peer::PeerId &peer_id,
        network::CollationVersion version) = 0;

    /// Handle incoming collation message.
    virtual void onIncomingMessage(
        const libp2p::peer::PeerId &peer_id,
        network::VersionedValidatorProtocolMessage &&validation_message) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_VALIDATION_OBSERVER_HPP
