/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_HPP
#define KAGOME_PEER_HPP

#include "libp2p/common_objects/multiaddress.hpp"
#include "libp2p/common_objects/multihash.hpp"

namespace libp2p {
  namespace common {
    class Peer {
     public:
      struct PeerInfo {
        const Multihash &peer_id;
        const Multiaddress &peer_address;
      };
    };
  }  // namespace common
}  // namespace libp2p

#endif  // KAGOME_PEER_HPP
