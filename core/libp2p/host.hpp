/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HOST_HPP
#define KAGOME_HOST_HPP

#include <functional>
#include <string>
#include <string_view>

#include <gsl/span>
#include <outcome/outcome.hpp>
#include "libp2p/connection/stream.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/network/network.hpp"
#include "libp2p/network/router.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/peer_repository.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p {
  /**
   * Entry point of Libp2p - through this class all interactions with the
   * library should go
   */
  struct Host {
    virtual ~Host() = default;

    /**
     * @brief Get a version of Libp2p, supported by this Host
     */
    virtual std::string_view getLibp2pVersion() const = 0;

    /**
     * @brief Get a version of this Libp2p client
     */
    virtual std::string_view getLibp2pClientVersion() const = 0;

    /**
     * @brief Get identifier of this Host
     */
    virtual peer::PeerId getId() const = 0;

    /**
     * @brief Get PeerInfo of this Host
     */
    virtual peer::PeerInfo getPeerInfo() const = 0;

    /**
     * @brief Get list of addresses this Host listens on
     */
    virtual gsl::span<const multi::Multiaddress> getListenAddresses() const = 0;

    /**
     * @brief Let Host handle given {@param proto} protocol
     * @param proto protocol to be handled
     * @param handler callback that is executed when some other Host creates
     * stream to our host with {@param proto} protocol.
     */
    virtual void setProtocolHandler(
        const peer::Protocol &proto,
        const std::function<connection::Stream::Handler> &handler) = 0;

    /**
     * @brief Let Host handle all protocols with prefix={@param prefix}, for
     * which predicate {@param predicate} is true.
     * param prefix prefix for the protocol. Example:
     * You want all streams, which came to Ping protocol with versions
     * between 1.5 and 2.0 to be handled by your handler; so, you may pass
     *  - prefix=/ping/1.
     *  - predicate=[](proto){return proto.version >= 1.5
     *      && proto.version < 2.0;}
     * @param handler of the arrived stream
     * @param predicate function that takes received protocol (/ping/1.0.0) and
     * returns true, if this protocol can be handled.
     */
    virtual void setProtocolHandler(
        const std::string &prefix,
        const std::function<connection::Stream::Handler> &handler,
        const std::function<bool(const peer::Protocol &)> &predicate) = 0;

    /**
     * @brief Initiates connection to the peer {@param p}. If connection exists,
     * does nothing, otherwise blocks until successful connection is created or
     * error happens.
     * @param p peer to connect. Addresses will be searched in PeerRepository.
     * If not found, will be searched using Routing module.
     * @return nothing on success, error otherwise.
     */
    virtual outcome::result<void> connect(const peer::PeerInfo &p) = 0;

    /**
     * @brief Open new stream to the peer {@param p} with protocol {@param
     * protocol}.
     * @param p stream will be opened with this peer
     * @param protocol "speak" using this protocol
     * @param handler callback, will be executed on successful stream creation
     * @return May return error if peer {@param p} is not known to this peer.
     */
    virtual outcome::result<void> newStream(
        const peer::PeerInfo &p, const peer::Protocol &protocol,
        const std::function<connection::Stream::Handler> &handler) = 0;

    /**
     * @brief Get a network component of the Host
     * @return reference to network
     */
    virtual const network::Network &network() const = 0;

    /**
     * @brief Get a peer repository of the Host
     * @return reference to repository
     */
    virtual peer::PeerRepository &peerRepository() const = 0;

    /**
     * @brief Get a router component of the Host
     * @return reference to router
     */
    virtual const network::Router &router() const = 0;
  };
}  // namespace libp2p

#endif  // KAGOME_HOST_HPP
