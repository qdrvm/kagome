/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/identity/peer_identity.hpp"

namespace libp2p::peer {
  PeerIdentity::PeerIdentity(std::string identity, PeerId id,
                             multi::Multiaddress address)
      : identity_{std::move(identity)},
        id_{std::move(id)},
        address_{std::move(address)} {}

  std::string_view PeerIdentity::getIdentity() const noexcept {
    return identity_;
  }

  const PeerId &PeerIdentity::getId() const noexcept {
    return id_;
  }

  const multi::Multiaddress &PeerIdentity::getAddress() const noexcept {
    return address_;
  }
}  // namespace libp2p::peer
