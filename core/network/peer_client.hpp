/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_PEER_CLIENT_HPP
#define KAGOME_CORE_NETWORK_PEER_CLIENT_HPP

#include "network/types/block_announce.hpp"
#include "network/types/blocks_request.hpp"
#include "network/types/blocks_response.hpp"

namespace kagome::network {

  /**
   * Networking abstraction for sending messages to particular peer
   */
  class PeerClient {
   public:
    virtual ~PeerClient() = default;
    /**
     * Request block
     */
    virtual void blocksRequest(
        BlocksRequest request,
        std::function<void(const outcome::result<BlocksResponse> &)> handler)
        const = 0;

    /**
     * Send block announce
     */
    virtual void blockAnnounce(
        BlockAnnounce block_announce,
        std::function<void(const outcome::result<void> &)> handler) const = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_PEER_CLIENT_HPP
