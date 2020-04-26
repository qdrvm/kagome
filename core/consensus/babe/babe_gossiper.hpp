/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_GOSSIPER_HPP
#define KAGOME_CONSENSUS_BABE_GOSSIPER_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "network/types/block_announce.hpp"

namespace kagome::consensus {
  /**
   * Sends messages, related to BABE, over the Gossip protocol
   */
  struct BabeGossiper {
    virtual ~BabeGossiper() = default;

    /**
     * Send BlockAnnounce message
     * @param announce to be sent
     */
    virtual void blockAnnounce(const network::BlockAnnounce &announce) = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_BABE_GOSSIPER_HPP
