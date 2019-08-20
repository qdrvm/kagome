/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_IMPL_COMMON_HPP
#define KAGOME_NETWORK_IMPL_COMMON_HPP

#include "libp2p/peer/protocol.hpp"

namespace kagome::network {
  const libp2p::peer::Protocol kSyncProtocol = "/polkadot-sync/1.0.0";
  const libp2p::peer::Protocol kGossipProtocol = "/polkadot-gossip/1.0.0";
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_IMPL_COMMON_HPP
