/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_TYPES_OWNPEERINFO
#define KAGOME_CORE_NETWORK_TYPES_OWNPEERINFO

#include <gsl/span>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/peer/peer_info.hpp>

#include "crypto/crypto_store.hpp"
#include "injector/get_peer_keypair.hpp"
#include "scale/libp2p_types.hpp"

namespace kagome::network {

  struct OwnPeerInfo : public libp2p::peer::PeerInfo {
    OwnPeerInfo(libp2p::peer::PeerId peer_id,
                std::vector<libp2p::multi::Multiaddress> public_addrs,
                std::vector<libp2p::multi::Multiaddress> listen_addrs)
        : PeerInfo{.id = std::move(peer_id),
                   .addresses = std::move(public_addrs)},
          listen_addresses{std::move(listen_addrs)} {}

    OwnPeerInfo(const application::AppConfiguration &config,
                libp2p::crypto::marshaller::KeyMarshaller &key_marshaller,
                const crypto::Ed25519Provider &crypto_provider,
                crypto::CryptoStore &crypto_store)
        : PeerInfo{.id = scale::PeerInfoSerializable::dummyPeerId(),
                   .addresses = {}},
          listen_addresses{config.listenAddresses()} {
      auto local_pair =
          injector::get_peer_keypair(config, crypto_provider, crypto_store);

      id = libp2p::peer::PeerId::fromPublicKey(
               key_marshaller.marshal(local_pair->publicKey).value())
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

#endif  // KAGOME_CORE_NETWORK_TYPES_OWNPEERINFO
