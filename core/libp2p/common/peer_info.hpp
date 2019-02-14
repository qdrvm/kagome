/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_INFO_HPP
#define KAGOME_PEER_INFO_HPP

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/multi/multihash.hpp"

namespace libp2p::common {
  /**
   * Information about the peer in the network
   */
  struct PeerInfo {
    multi::Multihash peer_id;
    multi::Multiaddress peer_address;
  };
}  // namespace libp2p::common

#endif  // KAGOME_PEER_INFO_HPP
