/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_HOST_HPP
#define KAGOME_LIBP2P_HOST_HPP

#include <memory>

#include <boost/asio/io_context.hpp>
#include "libp2p/config.hpp"
#include "libp2p/crypto/key.hpp"
#include "libp2p/crypto/random_generator.hpp"
#include "libp2p/dht/dht_adaptor.hpp"
#include "libp2p/discovery/discovery_adaptor.hpp"
#include "libp2p/host.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/muxer/muxer_adaptor.hpp"
#include "libp2p/peer/peer_repository.hpp"
#include "libp2p/routing/routing_adaptor.hpp"
#include "libp2p/security/security_adaptor.hpp"
#include "libp2p/transport/transport.hpp"

namespace libp2p {
  /**
   * Implements a builder pattern to help one create a Host instance, which can
   * be used to get access to Libp2p features
   */
  class HostBuilder {
   public:
    /**
     * Create a host builder
     * @param io_context to be used inside
     */
    explicit HostBuilder(boost::asio::io_context &io_context);

    /**
     * Create a host builder from the predefined config
     * @param io_context to be used inside
     * @param config to used inside
     */
    HostBuilder(boost::asio::io_context &io_context, Config config);

    /**
     * Set a long-term identity (keypair) of this Host
     * @param kp - keypair to be set
     * @return builder with a keypair set
     * @note any call after the first one replaces an old keypair
     * @note if no keypair is set, a random one will be generated
     */
    HostBuilder &setKeypair(const crypto::KeyPair &kp);

    /**
     * Set a cryptographic random generator; will be used to generate keypairs
     * and other cryptographic material
     * @param r - generator to be set
     * @return builder with a generator set
     * @note if no generator is set, the default one will be used
     */
    HostBuilder &setCSPRNG(detail::sptr<crypto::random::CSPRNG> r);

    /**
     * Set a usual random generator; will be used for timers and
     * non-cryptographic random numbers
     * @param r - generator to be set
     * @return builder with a generator set
     * @note if no generator is set, the default one will be used
     */
    HostBuilder &setPRNG(detail::sptr<crypto::random::RandomGenerator> r);

    /**
     * Add a routing adaptor - mechanism, which allows a Libp2p Node to find the
     * PeerInfo (id and address) of another Node
     * @param r - adaptor to be set
     * @return builder with the adaptor set
     * @note if no adaptor is set, the default one will be used
     */
    HostBuilder &setRoutingAdaptor(detail::sptr<routing::RoutingAdaptor> r);

    /**
     * Set a discovery adaptor - mechanism, which continuously find new peers
     * @param d - adaptor to be set
     * @return builder with the adaptor set
     * @note if no adaptor is set, the default one will be used
     */
    HostBuilder &setDiscoveryAdaptor(
        detail::sptr<discovery::DiscoveryAdaptor> d);

    /**
     * Set a peer repository - storage for various information, related to peers
     * of the network
     * @param p - repository to be set
     * @return builder with the adaptor set
     * @note if no repository is set, the default one will be used
     */
    HostBuilder &setPeerRepository(detail::sptr<peer::PeerRepository> p);

    /**
     * Add a transport to be supported by this Host; through transports nodes
     * are able to establish actual connections
     * @param tr - transport to be added
     * @return builder with that transport added
     * @note if no transports are added, TCP will be used
     */
    HostBuilder &addTransport(detail::sptr<transport::Transport> tr);

    /**
     * Add a muxer adaptor to be supported by this Host; muxer is a mechanism,
     * which allow to multiplex several logical stream over one physical
     * connection; adaptor is a strategy to update connection to the muxed one
     * @param mux - adaptor to be added
     * @return builder with the adaptor added
     * @note if no muxers are added, Yamux will be used
     */
    HostBuilder &addMuxerAdaptor(detail::sptr<muxer::MuxerAdaptor> mux);

    /**
     * Add a distributed hash table adaptor to be supported by this Host; it is
     * used to store information across several nodes in the netowrk
     * @param d - adaptor to be added
     * @return builder with the adaptor added
     * @note if no adaptors are added, the default one will be used
     */
    HostBuilder &addDHTAdaptor(detail::sptr<dht::DHTAdaptor> d);

    /**
     * Add a security adaptor to be supported by this Host; it is a strategy to
     * update connection to the secured one
     * @param s - adaptor to be added
     * @return builder with the adaptor added
     * @note if no adaptors are added, TLS will be used
     */
    HostBuilder &addSecurityAdaptor(detail::sptr<security::SecurityAdaptor> s);

    /**
     * Add an address, on which the host is going to listen to the network
     * @param address to be added
     * @return builder with the address added
     * @note if no multiaddresses to listen on were added, the host will not be
     * able to work as a server, accepting new connections or streams; still, it
     * will be able to initiate them
     */
    HostBuilder &addListenMultiaddr(const multi::Multiaddress &address);

    /**
     * @copydoc addListenMultiaddr(const multi::Multiaddress &address)
     */
    HostBuilder &addListenMultiaddr(std::string_view address);

    /**
     * Finish the build process
     * @return built host
     */
    Host build();

    // TODO(akvinikym) 23.05.19 PRE-183: implement this method
    /**
     * Deep clone state of this builder
     * @return clone of this builder
     */
    //    HostBuilder clone() const;

   private:
    /// state of the builder
    Config config_;

    boost::asio::io_context &io_context_;
  };
}  // namespace libp2p

#endif  // KAGOME_LIBP2P_HOST_HPP
