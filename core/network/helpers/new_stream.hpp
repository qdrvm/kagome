/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/host/host.hpp>

namespace kagome::network {
  /**
   * Use existing connection or dial using known addresses.
   */
  inline void newStream(libp2p::Host &host,
                        const libp2p::PeerId &peer,
                        libp2p::StreamProtocols protocols,
                        libp2p::StreamAndProtocolOrErrorCb cb) {
    libp2p::peer::PeerInfo info{peer, {}};
    if (auto r = host.getPeerRepository().getAddressRepository().getAddresses(
            peer)) {
      info.addresses = r.value();
    }
    host.newStream(info, std::move(protocols), std::move(cb));
  }
}  // namespace kagome::network
