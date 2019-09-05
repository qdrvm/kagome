/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SYNC_PROTOCOL_OBSERVER_HPP
#define KAGOME_SYNC_PROTOCOL_OBSERVER_HPP

#include <outcome/outcome.hpp>
#include "network/types/blocks_request.hpp"
#include "network/types/blocks_response.hpp"

namespace kagome::network {
  /**
   * Reactive part of Sync protocol
   */
  struct SyncProtocolObserver {
    virtual ~SyncProtocolObserver() = default;

    /**
     * Process a blocks request
     * @param request to be processed
     * @return blocks request or error
     */
    virtual outcome::result<BlocksResponse> onBlocksRequest(
        const BlocksRequest &request) const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_SYNC_PROTOCOL_OBSERVER_HPP
