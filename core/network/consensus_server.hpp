/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_SERVER_HPP
#define KAGOME_CONSENSUS_SERVER_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "network/types/block_request.hpp"
#include "network/types/block_response.hpp"

namespace kagome::network {
  /**
   * "Passive" part of the consensus RPC
   */
  struct ConsensusServer {
    virtual ~ConsensusServer() = default;

    using BlocksRequestHandler =
        std::function<outcome::result<BlocksResponse>(const BlocksRequest &)>;

    /**
     * Start accepting messages on this server
     */
    virtual void start() const;

    /**
     * Subscribe for the block requests
     * @param handler to be called, when a new block request arrives
     *
     * @note in case the method is called several times, only the last handler
     * will be called
     */
    virtual void setBlocksRequestHandler(BlocksRequestHandler handler) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_CONSENSUS_SERVER_HPP
