/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_TYPES_PEER_LIST_HPP
#define KAGOME_CORE_NETWORK_TYPES_PEER_LIST_HPP

#include <libp2p/peer/peer_info.hpp>

namespace kagome::network {

  struct PeerList {
    std::vector<libp2p::peer::PeerInfo> peers;
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_TYPES_PEER_LIST_HPP
