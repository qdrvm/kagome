/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_SERVER_HPP
#define KAGOME_GOSSIPER_SERVER_HPP

#include <functional>

#include "network/types/block_announce.hpp"

namespace kagome::network {
  /**
   * "Passive" part of the consensus gossiping
   */
  struct GossiperServer {
    virtual ~GossiperServer() = default;

    /**
     * Start accepting messages on this server
     */
    virtual void start() = 0;

    using BlockAnnounceHandler = std::function<void(const BlockAnnounce &)>;

    /**
     * Process block announcement
     * @param handler to be called, when a block announce arrives
     *
     * @note in case the method is called several times, only the last handler
     * will be called
     */
    virtual void setBlockAnnounceHandler(BlockAnnounceHandler handler) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_SERVER_HPP
