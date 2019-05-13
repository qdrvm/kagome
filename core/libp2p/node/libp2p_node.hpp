/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_NODE_HPP
#define KAGOME_LIBP2P_NODE_HPP

#include <functional>
#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/muxer/muxer_config.hpp"
#include "libp2p/muxer/stream_muxer.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "libp2p/routing/router.hpp"
#include "libp2p/security/connection_encryptor.hpp"
#include "libp2p/store/record_store.hpp"
#include "libp2p/stream/stream.hpp"
#include "libp2p/swarm/swarm.hpp"
#include "libp2p/transport/transport.hpp"

namespace libp2p::node {
  /**
   * Provides a way to instantiate a libp2p instance via joining different
   * components to it and dial to the peer
   */
  class Libp2pNode {
   public:
    /**
     * Start the node
     * @return void, if start is successful, error otherwise
     * @note blocks
     */
    outcome::result<void> start();

    /**
     * Add a transport to this node
     * @param transport to be added
     */
    virtual void addTransport(std::unique_ptr<transport::Transport> transport);

    /// muxers, which are currently supported in this implementation
    enum class SupportedMuxers { YAMUX };

    /**
     * Add a muxer to this node
     */
    virtual void addMuxer(SupportedMuxers muxer_type,
                          muxer::MuxerConfig config);

    /**
     * Add a connection encryptor to this node
     * @param conn_encryptor to be added
     */
    virtual void addEncryptor(
        std::unique_ptr<security::ConnectionEncryptor> conn_encryptor);

    /**
     * Add a swarm to this node
     * @param swarm to be added
     */
    virtual void addSwarm(std::unique_ptr<swarm::Swarm> swarm) = 0;

    /**
     * Add a router to this node
     * @param routing to be added
     */
    virtual void addPeerRouting(std::unique_ptr<routing::Router> router) = 0;

    /**
     * Add a record store to this node
     * @param record store to be added
     */
    virtual void addRecordStore(std::unique_ptr<store::RecordStore> store) = 0;

    using DialCallback =
        std::function<void(outcome::result<std::unique_ptr<stream::Stream>>)>;

    /**
     * Dial to the peer via its info
     * @param peer_info of the peer
     * @param protocol, over which to dial
     * @param cb, to be called, when a connection is established, or error
     * happens
     */
    virtual void dial(const peer::PeerInfo &peer_info,
                      const peer::Protocol &protocol, DialCallback cb) = 0;

    /**
     * Get PeerInfo of this node
     * @return peer info
     */
    virtual const peer::PeerInfo &peerInfo() const = 0;

    using HandleCallback =
        std::function<void(std::unique_ptr<stream::Stream>, peer::PeerId)>;

    /**
     * Handle a specified protocol
     * @param protocol to be handled
     * @param cb to be called, when a connection over the protocol is received
     */
    virtual void handle(const peer::Protocol &protocol, HandleCallback cb);

    /**
     * Accept connections on the specified address
     * @param address to listen
     */
    virtual void listen(const multi::Multiaddress &address);

    virtual ~Libp2pNode() = default;
  };
}  // namespace libp2p::node

#endif  // KAGOME_LIBP2P_NODE_HPP
