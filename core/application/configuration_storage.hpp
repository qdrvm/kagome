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

#ifndef KAGOME_CONFIGURATION_STORAGE_HPP
#define KAGOME_CONFIGURATION_STORAGE_HPP

#include <libp2p/peer/peer_info.hpp>
#include "application/genesis_raw_config.hpp"
#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"
#include "network/types/peer_list.hpp"
#include "primitives/block.hpp"

namespace kagome::application {

  /**
   * Stores configuration of a kagome application and provides convenience
   * methods for accessing config parameters
   */
  class ConfigurationStorage {
   public:
    virtual ~ConfigurationStorage() = default;

    /**
     * @return genesis block of the chain
     */
    virtual GenesisRawConfig getGenesis() const = 0;

    /**
     * Return ids of peer nodes of the current node
     */
    virtual network::PeerList getBootNodes() const = 0;
  };

}  // namespace kagome::application

#endif  // KAGOME_CONFIGURATION_STORAGE_HPP
