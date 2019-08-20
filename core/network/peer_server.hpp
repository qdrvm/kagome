/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_PEER_SERVER_HPP
#define KAGOME_CORE_NETWORK_PEER_SERVER_HPP

#include "network/types/block_announce.hpp"
#include "network/types/block_request.hpp"
#include "network/types/block_response.hpp"

namespace kagome::network {

  /**
   * Networking abstraction for receiving and processing messages as local peer
   */
  class PeerServer {
   public:
    virtual ~PeerServer() = default;

    using BlocksRequestHandler =
        std::function<outcome::result<BlockResponse>(const BlockRequest &)>;

    /**
     * Subscribe for the block requests
     * @param handler to be called, when a new block request arrives
     *
     * @note in case the method is called several times, only the last handler
     * will be called
     */
    virtual void onBlocksRequest(BlocksRequestHandler handler) const = 0;

    using BlockAnnounceHandler = std::function<void(const BlockAnnounce &)>;

    /**
     * Process block announcement
     * @param handler to be called, when a block announce arrives
     *
     * @note in case the method is called several times, only the last handler
     * will be called
     */
    virtual void onBlockAnnounce(BlockAnnounceHandler handler) const = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_PEER_SERVER_HPP
