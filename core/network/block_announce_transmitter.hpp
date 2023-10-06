/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/types/block_announce.hpp"

namespace kagome::network {
  /**
   * Sends block announce
   */
  class BlockAnnounceTransmitter {
   public:
    virtual ~BlockAnnounceTransmitter() = default;

    /**
     * Send BlockAnnounce message
     * @param announce to be sent
     */
    virtual void blockAnnounce(network::BlockAnnounce &&announce) = 0;
  };
}  // namespace kagome::network
