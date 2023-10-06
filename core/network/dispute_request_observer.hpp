/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_DISPUTEREQUESTOBSERVER
#define KAGOME_NETWORK_DISPUTEREQUESTOBSERVER

#include "outcome/outcome.hpp"

#include <libp2p/peer/peer_id.hpp>

namespace kagome::network {
  struct DisputeMessage;
}

namespace kagome::network {

  class DisputeRequestObserver {
   public:
    virtual ~DisputeRequestObserver() = default;

    virtual void onDisputeRequest(
        const libp2p::peer::PeerId &peer_id,
        const DisputeMessage &request,
        std::function<void(outcome::result<void>)> &&cb) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_DISPUTEREQUESTOBSERVER
