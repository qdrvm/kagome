/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_HPP
#define KAGOME_GOSSIPER_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "network/types/block_announce.hpp"

namespace kagome::network {
  /**
   * Allows to send messages to the known network using gossip protocols
   */
  struct Gossiper {
    virtual ~Gossiper() = default;

    /**
     * Send block announce
     * @param block_announce - message with the block
     * @param cb, which is called, when the message is delivered
     */
    virtual void blockAnnounce(
        BlockAnnounce block_announce,
        std::function<void(const outcome::result<void> &)> cb) const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_HPP
