/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_PEER_SERVER_HPP
#define KAGOME_CORE_NETWORK_PEER_SERVER_HPP

#include "network/types/block_announce.hpp"
#include "network/types/block_request.hpp"

namespace kagome::network {

  /**
   * Networking abstraction for receiving and processing messages as local peer
   */
  class PeerServer {
   public:
    /**
     * Process block announcement
     */
    virtual void onBlockAnnounce(
        BlockAnnounce block_announce,
        std::function<void(const outcome::result<void> &)>) const = 0;

    /**
     * Process block request
     */
    virtual void onRequestBlock(
        BlockRequest request,
        std::function<void(const outcome::result<void> &)>) const = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_PEER_SERVER_HPP
