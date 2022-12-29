/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_GRANDPAROTOCOL
#define KAGOME_NETWORK_GRANDPAROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/app_configuration.hpp"
#include "consensus/grandpa/grandpa_observer.hpp"
#include "containers/objects_cache.hpp"
#include "log/logger.hpp"
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/types/own_peer_info.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::network {

  KAGOME_DECLARE_CACHE(GrandpaProtocol, KAGOME_CACHE_UNIT(GrandpaMessage));

  class GrandpaProtocol final
      : public ProtocolBase,
        public std::enable_shared_from_this<GrandpaProtocol>,
        NonCopyable,
        NonMovable {
   public:
    GrandpaProtocol() = delete;
    ~GrandpaProtocol() override = default;

    GrandpaProtocol(
        libp2p::Host &host,
        std::shared_ptr<boost::asio::io_context> io_context,
        const application::AppConfiguration &app_config,
        std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
        const OwnPeerInfo &own_info,
        std::shared_ptr<StreamEngine> stream_engine,
        std::shared_ptr<PeerManager> peer_manager,
        const primitives::BlockHash &genesis_hash,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler);

    /**
     * Sets handler for `parytytech/grandpa/1` protocol
     * @return true if handler set successfully
     */
    bool start() override;
    bool stop() override;

    const std::string &protocolName() const override;

    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

    void vote(network::GrandpaVote &&vote_message,
              std::optional<const libp2p::peer::PeerId> peer_id);
    void neighbor(GrandpaNeighborMessage &&msg);
    void finalize(FullCommitMessage &&msg,
                  std::optional<const libp2p::peer::PeerId> peer_id);
    void catchUpRequest(const libp2p::peer::PeerId &peer_id,
                        CatchUpRequest &&catch_up_request);
    void catchUpResponse(const libp2p::peer::PeerId &peer_id,
                         CatchUpResponse &&catch_up_response);

   private:
    const static inline auto kGrandpaProtocolName = "GrandpaProtocol"s;
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

    /// Node should send catch-up requests rarely to be polite, because
    /// processing of them consume more enough resources.
    /// How long replying outgoing catch-up requests must be suppressed
    static constexpr std::chrono::milliseconds kRecentnessDuration =
        std::chrono::seconds(300);

    ProtocolBaseImpl base_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    const application::AppConfiguration &app_config_;
    std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer_;
    const OwnPeerInfo &own_info_;
    std::shared_ptr<StreamEngine> stream_engine_;
    std::shared_ptr<PeerManager> peer_manager_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;

    std::set<std::tuple<consensus::grandpa::RoundNumber,
                        consensus::grandpa::VoterSetId>>
        recent_catchup_requests_by_round_;

    std::set<libp2p::peer::PeerId> recent_catchup_requests_by_peer_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_GRANDPAROTOCOL
