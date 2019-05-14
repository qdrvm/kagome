/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_Host_HPP
#define KAGOME_LIBP2P_Host_HPP

#include <memory>

#include "libp2p/config.hpp"
#include "libp2p/crypto/key.hpp"
#include "libp2p/crypto/random_generator.hpp"
#include "libp2p/dht/dht.hpp"
#include "libp2p/discovery/peer_discovery.hpp"
#include "libp2p/host.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/muxer/connection_muxer.hpp"
#include "libp2p/routing/router.hpp"
#include "libp2p/security/connection_security.hpp"
#include "libp2p/transport/transport.hpp"

namespace libp2p {

  class HostBuilder {
   public:
    // long-term identity/keypair for this Host
    HostBuilder &setKeypair(const crypto::KeyPair &kp);

    // rng for generating keypairs, cryptographic material
    HostBuilder &setCSPRNG(uptr<crypto::random::CSPRNG> r);

    // rng for timers and non-cryptographic random numbers
    HostBuilder &setPRNG(uptr<crypto::random::RandomGenerator> r);

    // add supported routing mechanisms
    //
    // A Peer Routing module offers a way for a libp2p Node to find the PeerInfo
    // of another Node, so that it can dial that node. In its most pure form, a
    // Peer Routing module should have an interface that takes a 'key', and
    // returns a set of PeerInfos
    HostBuilder &setPeerRouting(uptr<routing::PeerRouting> r);

    // add supported discovery mechanisms
    //
    // A Peer Discovery module interface should return PeerInfo objects, as it
    // finds new peers to be considered by our Peer Routing modules.
    HostBuilder &setPeerDiscovery(uptr<discovery::PeerDiscovery> d);

    // add supported transports
    HostBuilder &addTransport(uptr<transport::Transport> tr);

    // add supported muxers
    HostBuilder &addStreamMuxer(uptr<muxer::StreamMuxer> mux);

    // add distributed hash table provider
    HostBuilder &addDHT(uptr<dht::DHT> d);

    // add supported security options
    HostBuilder &addConnectionEncryption(uptr<security::ConnectionSecurity> s);

    // Host will listen on this addresses
    HostBuilder &addListenAddress(const multi::Multiaddress &address);
    HostBuilder &addListenAddress(const std::string_view &address);

    // create Host with given config
    Host build();

    // clone state of this builder
    HostBuilder clone();

   private:
    Config config_;
  };
}  // namespace libp2p

#endif  // KAGOME_LIBP2P_Host_HPP
