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

#ifndef KAGOME_CORE_NETWORK_TYPES_PEER_LIST_HPP
#define KAGOME_CORE_NETWORK_TYPES_PEER_LIST_HPP

#include <libp2p/peer/peer_info.hpp>

namespace kagome::network {

  struct PeerList {
    std::vector<libp2p::peer::PeerInfo> peers;

    bool operator==(const PeerList &rhs) const {
      return peers == rhs.peers;
    }
    bool operator!=(const PeerList &rhs) const {
      return not operator==(rhs);
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_TYPES_PEER_LIST_HPP
