/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/protocol_base.hpp"

#include "consensus/beefy/types.hpp"

namespace kagome::network {
  class BeefyProtocol : public virtual ProtocolBase {
   public:
    virtual void broadcast(
        std::shared_ptr<consensus::beefy::BeefyGossipMessage> message) = 0;
  };
}  // namespace kagome::network
