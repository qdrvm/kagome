/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_REQ_COLLATION_OBSERVER_HPP
#define KAGOME_REQ_COLLATION_OBSERVER_HPP

#include <libp2p/peer/peer_id.hpp>

#include "consensus/grandpa/common.hpp"
#include "network/types/collator_messages.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to collation protocol
   */
  class ReqCollationObserver {
   public:
    virtual ~ReqCollationObserver() = default;

    /**
     * Since we are not collator node. Received request should make call to
     * subsystem to decrease rank of the requested node.
     */
    virtual outcome::result<CollationFetchingResponse> OnCollationRequest(
        CollationFetchingRequest request) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_REQ_COLLATION_OBSERVER_HPP
