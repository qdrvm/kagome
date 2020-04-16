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

#ifndef KAGOME_SYNC_CLIENTS_SET_HPP
#define KAGOME_SYNC_CLIENTS_SET_HPP

#include <memory>
#include <unordered_set>

#include "network/sync_protocol_client.hpp"

namespace kagome::network {
  /**
   * Keeps all known Sync clients
   */
  struct SyncClientsSet {
    std::unordered_set<std::shared_ptr<SyncProtocolClient>> clients;
  };
}  // namespace kagome::network

#endif  // KAGOME_SYNC_CLIENTS_SET_HPP
