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

#ifndef KAGOME_SYNCHRONIZER_HPP
#define KAGOME_SYNCHRONIZER_HPP

#include "network/sync_protocol_client.hpp"
#include "network/sync_protocol_observer.hpp"
#include "primitives/block_header.hpp"

namespace kagome::consensus {
  /**
   * Synchronizer, which retrieves blocks from other peers; mostly used to
   * *synchronize* this node's state with the state of other nodes, for example,
   * if this node does not have some blocks
   */
  struct Synchronizer : public network::SyncProtocolClient,
                        public network::SyncProtocolObserver {};
}  // namespace kagome::consensus

#endif  // KAGOME_SYNCHRONIZER_HPP
