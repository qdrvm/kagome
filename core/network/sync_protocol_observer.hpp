/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
