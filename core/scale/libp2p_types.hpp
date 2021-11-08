/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_LIBP2P_TYPES_HPP
#define KAGOME_CORE_SCALE_LIBP2P_TYPES_HPP

#include <libp2p/peer/peer_info.hpp>
#include "scale/scale.hpp"

namespace scale {

  class PeerInfoSerializable : public libp2p::peer::PeerInfo {
   public:
    PeerInfoSerializable();

    static libp2p::peer::PeerId dummyPeerId();
  };

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                 const libp2p::peer::PeerInfo &peer_info);

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                 libp2p::peer::PeerInfo &peer_info);

}  // namespace scale

#endif  // KAGOME_CORE_SCALE_LIBP2P_TYPES_HPP
