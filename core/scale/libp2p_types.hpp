/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_LIBP2P_TYPES_HPP
#define KAGOME_CORE_SCALE_LIBP2P_TYPES_HPP

#include <algorithm>
#include <exception>

#include <libp2p/peer/peer_info.hpp>
#include "scale/scale.hpp"

namespace kagome::scale {

  class PeerInfoSerializable : public libp2p::peer::PeerInfo {
   public:
    PeerInfoSerializable();

    static libp2p::peer::PeerId dummyPeerId();
  };

  ScaleEncoderStream &operator<<(ScaleEncoderStream &s,
                                 const libp2p::peer::PeerInfo &peer_info);

  ScaleDecoderStream &operator>>(ScaleDecoderStream &s,
                                 libp2p::peer::PeerInfo &peer_info);

}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_LIBP2P_TYPES_HPP
