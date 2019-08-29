/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_CLIENT_HPP
#define KAGOME_CONSENSUS_CLIENT_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "network/types/block_request.hpp"
#include "network/types/block_response.hpp"

namespace kagome::network {
  /**
   * "Active" part of the consensus RPC
   */
  struct ConsensusClient {
    virtual ~ConsensusClient() = default;

    using BlocksResponseHandler =
        std::function<void(const outcome::result<BlocksResponse> &)>;

    /**
     * Request blocks
     * @param request - block request
     * @param cb - function, which is called, when a corresponding response
     * arrives
     */
    virtual void blocksRequest(BlocksRequest request,
                               BlocksResponseHandler cb) const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_CONSENSUS_CLIENT_HPP
