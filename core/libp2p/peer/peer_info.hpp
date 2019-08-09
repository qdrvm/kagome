/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_INFO_HPP
#define KAGOME_PEER_INFO_HPP

#include <vector>

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/peer_id.hpp"

namespace libp2p::peer {

  struct PeerInfo {
    PeerId id;
    std::vector<multi::Multiaddress> addresses;

    bool operator==(const PeerInfo &other) const {
      return id == other.id && addresses == other.addresses;
    }
    bool operator!=(const PeerInfo &other) const {
      return !(*this == other);
    }
  };

}  // namespace libp2p::peer

#endif  // KAGOME_PEER_INFO_HPP
