/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>

#include "scale/kagome_scale.hpp"

namespace scale {

  class PeerInfoSerializable : public libp2p::peer::PeerInfo {
   public:
    PeerInfoSerializable();

    static libp2p::peer::PeerId dummyPeerId();
  };

  void encode(const libp2p::peer::PeerInfo &peer_info,
              ::scale::Encoder &encoder);

  void decode(libp2p::peer::PeerInfo &peer_info, ::scale::Decoder &decoder);

}  // namespace scale
