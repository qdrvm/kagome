/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_PEER_SERVER_HPP
#define KAGOME_CORE_NETWORK_PEER_SERVER_HPP

#include "network/types/block_announce.hpp"
#include "network/types/blocks_request.hpp"
#include "network/types/blocks_response.hpp"

namespace kagome::network {

  /**
   * Networking abstraction for receiving and processing messages as local peer
   */
  class PeerServer {
   public:
    virtual ~PeerServer() = default;

    /**
     * Subscribe for the block requests
     */
    virtual void onBlocksRequest(
        std::function<outcome::result<BlocksResponse>(const BlocksRequest &)>)
        const = 0;

    /**
     * Process block announcement
     */
    virtual void onBlockAnnounce(
        std::function<void(const BlockAnnounce &)>) const = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_PEER_SERVER_HPP
