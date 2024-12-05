//
// Created by taisei on 03.12.2024.
//

#pragma once

#include "authority_discovery/query/authority_peer_info.hpp"
#include "authority_discovery/timestamp.hpp"
#include "common/buffer.hpp"
#include "scale/libp2p_types.hpp"

namespace kagome::authority_discovery {
  struct AuthorityPeerInfo {
    SCALE_TIE(3);
    common::Buffer raw{};
    std::optional<TimestampScale> time{};
    scale::PeerInfoSerializable peer{};
  };


}  // namespace kagome::authority_discovery
