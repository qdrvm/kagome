/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace libp2p::peer {
  class PeerId;
}  // namespace libp2p::peer
namespace libp2p {
  using peer::PeerId;
}

namespace kagome::network {
  class CanDisconnect {
   public:
    virtual ~CanDisconnect() = default;

    virtual bool can_disconnect(const libp2p::PeerId &) const = 0;
  };
}  // namespace kagome::network
