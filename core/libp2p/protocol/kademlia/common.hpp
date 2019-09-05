/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KADEMLIA_COMMON_HPP
#define KAGOME_KADEMLIA_COMMON_HPP

#include <string>
#include <unordered_set>

#include "common/blob.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/protocol/kademlia/node_id.hpp"

namespace libp2p::protocol::kademlia {

  using kagome::common::Hash256;

  /// DHT key
  using Key = std::vector<uint8_t>;

  /// DHT value
  using Value = std::vector<uint8_t>;

  /// Set of peer Ids
  using PeerIdSet = std::unordered_set<peer::PeerId>;

  /// Vector of peer Ids
  using PeerIdVec = std::vector<peer::PeerId>;

  /// Vector of peer Infos
  using PeerInfoVec = std::vector<peer::PeerInfo>;

  /// Vector of Node Ids
  using NodeIdVec = std::vector<NodeId>;

  /// Content Id
  struct Cid {
    // TODO(warchant): tbd
  };

  struct ReceivedValue {
    Value value;
    peer::PeerId from;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // KAGOME_KADEMLIA_COMMON_HPP
