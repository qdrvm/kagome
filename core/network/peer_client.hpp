/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_PEER_CLIENT_HPP
#define KAGOME_CORE_NETWORK_PEER_CLIENT_HPP

#include <outcome/outcome.hpp>
#include "network/types/block_announce.hpp"
#include "network/types/block_request.hpp"
#include "network/types/block_response.hpp"

namespace kagome::network {

  /**
   * Networking abstraction for sending messages to particular peer
   */
  class PeerClient {
   public:
    virtual ~PeerClient() = default;

    using BlockResponseHandler =
        std::function<void(const outcome::result<BlocksResponse> &)>;

    /**
     * Request block
     * @param request - block request
     * @param handler - function, which is called, when a corresponding response
     * arrives
     */
    virtual void blocksRequest(BlocksRequest request,
                               BlockResponseHandler handler) const = 0;

    /**
     * Send block announce
     * @param block_announce - message with the block
     * @param handler, which is called, when the message is delivered
     */
    virtual void blockAnnounce(
        BlockAnnounce block_announce,
        std::function<void(const outcome::result<void> &)> handler) const = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_PEER_CLIENT_HPP
