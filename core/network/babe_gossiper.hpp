/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_GOSSIPER_HPP
#define KAGOME_BABE_GOSSIPER_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "network/types/block_announce.hpp"

namespace kagome::network {
  /**
   * Sends messages, related to BABE, over the Gossip protocol
   */
  struct BabeGossiper {
    virtual ~BabeGossiper() = default;

    /**
     * Send BlockAnnounce message
     * @param announce to be sent
     * @param cb to be called, when announce is sent, or error happens
     */
    virtual void blockAnnounce(
        const BlockAnnounce &announce,
        std::function<void(outcome::result<void>)> cb) const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_BABE_GOSSIPER_HPP
