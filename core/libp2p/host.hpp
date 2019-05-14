/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HOST_HPP
#define KAGOME_HOST_HPP

#include <functional>

#include <gsl/span>
#include <outcome/outcome.hpp>
#include "libp2p/config.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/peer/peer_repository.hpp"
#include "libp2p/peer/protocol.hpp"
#include "libp2p/stream/stream.hpp"

#include <boost/asio.hpp>

namespace libp2p {

  class Host {
   public:
    using StreamHandler = void(std::unique_ptr<stream::Stream>);

    /**
     * @brief Get identifier of this Host
     */
    peer::PeerId getId() const noexcept;

    /**
     * @brief Get list of addresses this Host listens on
     */
    gsl::span<const multi::Multiaddress> getListenAddresses();

    /**
     * @brief Let Host handle given {@param proto} protocol
     * @param proto protocol to be handled
     * @param handler callback that is executed when some other Host creates
     * stream to our host with {@param proto} protocol.
     */
    void setProtocolHandler(const peer::Protocol &proto,
                            const std::function<StreamHandler> &handler);

    void setProtocolHandlerMatch(const std::function<,
                            const std::function<StreamHandler> &handler);
    /**
     * @brief Initiates connection to the peer {@param p}. If connection exists,
     * does nothing, otherwise blocks until successful connection is created or
     * error happens.
     * @param p peer to connect. Addresses will be searched in PeerRepository.
     * If not found, will be searched using Routing module.
     * @return nothing on success, error otherwise.
     */
    outcome::result<void> connect(const peer::PeerId &p);
    outcome::result<void> connect(const multi::Multiaddress &ma);

    /**
     * @brief Open new stream to the peer {@param p} with protocol {@param
     * protocol}.
     * @param p stream will be opened with this peer
     * @param protocol "speak" using this protocol
     * @param handler callback, will be executed on successful stream creation
     * @return May return error if peer {@param p} is not known to this peer.
     */
    outcome::result<void> newStream(
        const peer::PeerId &p, const peer::Protocol &protocol,
        const std::function<StreamHandler> &handler);

    ///////////////////////// adequate code boundary /////////////////////////
    // inadequate

    peer::PeerRepository &getPeerRepository() const;

    outcome::result<void> addKnownPeer(const peer::PeerId &p,
                                       gsl::span<const multi::Multiaddress> mas,
                                       std::chrono::milliseconds ttl);

   private:
    const peer::PeerId id_;
    Config config_;
  };

}  // namespace libp2p

#endif  // KAGOME_HOST_HPP
