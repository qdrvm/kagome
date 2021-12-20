/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_GRANDPAROTOCOL
#define KAGOME_NETWORK_GRANDPAROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/app_configuration.hpp"
#include "consensus/grandpa/grandpa_observer.hpp"
#include "containers/objects_cache.hpp"
#include "log/logger.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/types/own_peer_info.hpp"

namespace kagome::network {

  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;

  KAGOME_DECLARE_CACHE(GrandpaProtocol, KAGOME_CACHE_UNIT(GrandpaMessage));

  class GrandpaProtocol final
      : public ProtocolBase,
        public std::enable_shared_from_this<GrandpaProtocol> {
   public:
    GrandpaProtocol() = delete;
    GrandpaProtocol(GrandpaProtocol &&) noexcept = delete;
    GrandpaProtocol(const GrandpaProtocol &) = delete;
    ~GrandpaProtocol() override = default;
    GrandpaProtocol &operator=(GrandpaProtocol &&) noexcept = delete;
    GrandpaProtocol &operator=(GrandpaProtocol const &) = delete;

    GrandpaProtocol(
        libp2p::Host &host,
        std::shared_ptr<boost::asio::io_context> io_context,
        const application::AppConfiguration &app_config,
        std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
        const OwnPeerInfo &own_info,
        std::shared_ptr<StreamEngine> stream_engine,
        std::shared_ptr<PeerManager> peer_manager);

    const Protocol &protocol() const override {
      return protocol_;
    }

    bool start() override;
    bool stop() override;

    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

    void vote(network::GrandpaVote &&vote_message);
    void neighbor(GrandpaNeighborMessage &&msg);
    void finalize(FullCommitMessage &&msg);
    void catchUpRequest(const libp2p::peer::PeerId &peer_id,
                        CatchUpRequest &&catch_up_request);
    void catchUpResponse(const libp2p::peer::PeerId &peer_id,
                         CatchUpResponse &&catch_up_response);

   private:
    enum class Direction { INCOMING, OUTGOING };
    void readHandshake(std::shared_ptr<Stream> stream,
                       Direction direction,
                       std::function<void(outcome::result<void>)> &&cb);

    void writeHandshake(std::shared_ptr<Stream> stream,
                        Direction direction,
                        std::function<void(outcome::result<void>)> &&cb);

    void read(std::shared_ptr<Stream> stream);

    void write(
        std::shared_ptr<Stream> stream,
        const int &msg,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb);

    libp2p::Host &host_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    const application::AppConfiguration &app_config_;
    std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer_;
    const OwnPeerInfo &own_info_;
    std::shared_ptr<StreamEngine> stream_engine_;
    std::shared_ptr<PeerManager> peer_manager_;
    const libp2p::peer::Protocol protocol_;
    log::Logger log_ = log::createLogger("GrandpaProtocol", "grandpa_protocol");
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_GRANDPAROTOCOL
