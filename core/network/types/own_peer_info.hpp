/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gsl/span>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/peer/peer_info.hpp>

#include "application/app_configuration.hpp"
#include "crypto/crypto_store.hpp"
#include "scale/libp2p_types.hpp"

namespace kagome::network {

  struct OwnPeerInfo : public libp2p::peer::PeerInfo {
    OwnPeerInfo(const application::AppConfiguration &config,
                libp2p::crypto::marshaller::KeyMarshaller &key_marshaller,
                const libp2p::crypto::KeyPair &local_pair)
        : PeerInfo{.id = scale::PeerInfoSerializable::dummyPeerId(),
                   .addresses = {}},
          listen_addresses{config.listenAddresses()} {
      id = libp2p::peer::PeerId::fromPublicKey(
               key_marshaller.marshal(local_pair.publicKey).value())
               .value();

      addresses = config.publicAddresses();

      auto log = log::createLogger("Injector", "injector");
      for (auto &addr : listen_addresses) {
        SL_DEBUG(
            log, "Peer listening on multiaddr: {}", addr.getStringAddress());
      }
      for (auto &addr : addresses) {
        SL_DEBUG(log, "Peer public multiaddr: {}", addr.getStringAddress());
      }
    }

    bool operator==(const OwnPeerInfo &other) const {
      return id == other.id && addresses == other.addresses
          && listen_addresses == other.listen_addresses;
    }

    bool operator!=(const OwnPeerInfo &other) const {
      return !(*this == other);
    }

    std::vector<libp2p::multi::Multiaddress> listen_addresses;
  };

}  // namespace kagome::network
