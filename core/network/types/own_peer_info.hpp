/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_TYPES_OWNPEERINFO
#define KAGOME_CORE_NETWORK_TYPES_OWNPEERINFO

#include <libp2p/peer/peer_info.hpp>

namespace kagome::network {

  struct OwnPeerInfo : public libp2p::peer::PeerInfo {
    OwnPeerInfo(libp2p::peer::PeerId peer_id,
                std::vector<libp2p::multi::Multiaddress> public_addrs,
                std::vector<libp2p::multi::Multiaddress> listen_addrs)
        : PeerInfo{.id = std::move(peer_id),
                   .addresses = std::move(public_addrs)},
          listen_addresses{std::move(listen_addrs)} {}

    bool operator==(const OwnPeerInfo &other) const {
      return id == other.id && addresses == other.addresses
             && listen_addresses == other.listen_addresses;
    }

    bool operator!=(const OwnPeerInfo &other) const {
      return !(*this == other);
    }

    std::vector<libp2p::multi::Multiaddress> listen_addresses;
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_TYPES_OWNPEERINFO
