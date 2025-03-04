/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "authority_discovery/query/authority_peer_info.hpp"
#include "authority_discovery/timestamp.hpp"
#include "common/buffer.hpp"
#include "scale/libp2p_types.hpp"

namespace kagome::authority_discovery {
  struct AuthorityPeerInfo {
    common::Buffer raw{};
    std::optional<Timestamp> time{};
    scale::PeerInfoSerializable peer{};
  };

}  // namespace kagome::authority_discovery
