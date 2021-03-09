/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_BROADCAST_HPP
#define KAGOME_GOSSIPER_BROADCAST_HPP

#include "network/gossiper.hpp"

#include <gsl/span>
#include <unordered_map>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/peer/protocol.hpp>

#include "common/logger.hpp"
#include "containers/objects_cache.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/types/bootstrap_nodes.hpp"
#include "network/types/gossip_message.hpp"
#include "network/types/no_data_message.hpp"
#include "primitives/event_types.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"

namespace kagome::application {
  class ChainSpec;
}

namespace kagome::network {
  KAGOME_DECLARE_CACHE(stream_engine,
                       KAGOME_CACHE_UNIT(GossipMessage),
                       KAGOME_CACHE_UNIT(NoData),
                       KAGOME_CACHE_UNIT(BlockAnnounce),
                       KAGOME_CACHE_UNIT(PropagatedExtrinsics));

  /**
   * Sends gossip messages using broadcast strategy
   */
  class GossiperBroadcast
      : public Gossiper,
        public std::enable_shared_from_this<GossiperBroadcast> {
    using Precommit = consensus::grandpa::Precommit;
    using Prevote = consensus::grandpa::Prevote;
    using PrimaryPropose = consensus::grandpa::PrimaryPropose;

   public:
    GossiperBroadcast(
        StreamEngine::StreamEnginePtr stream_engine,
        std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
            extrinsic_events_engine,
        std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
            ext_event_key_repo_,
        std::shared_ptr<kagome::application::ChainSpec> config);

    ~GossiperBroadcast() override = default;

    void storeSelfPeerInfo(const libp2p::peer::PeerInfo &self_info) override;

    void propagateTransactions(
        gsl::span<const primitives::Transaction> txs) override;

    void blockAnnounce(const BlockAnnounce &announce) override;

    void vote(const network::GrandpaVoteMessage &msg) override;

    void finalize(const network::GrandpaPreCommit &msg) override;

    void catchUpRequest(const libp2p::peer::PeerId &peer_id,
                        const CatchUpRequest &catch_up_request) override;

    void catchUpResponse(const libp2p::peer::PeerId &peer_id,
                         const CatchUpResponse &catch_up_response) override;

   private:
    template <typename T>
    void send(const libp2p::peer::PeerId &peer_id,
              const libp2p::peer::Protocol &protocol,
              T &&msg) {
      auto shared_msg = KAGOME_EXTRACT_SHARED_CACHE(
          stream_engine, typename std::decay<decltype(msg)>::type);
      (*shared_msg) = std::forward<T>(msg);
      stream_engine_->send<typename std::decay<decltype(msg)>::type, NoData>(
          peer_id, protocol, std::move(shared_msg), boost::none);
    }

    template <typename T>
    void broadcast(const libp2p::peer::Protocol &protocol, T &&msg) {
      auto shared_msg = KAGOME_EXTRACT_SHARED_CACHE(
          stream_engine, typename std::decay<decltype(msg)>::type);
      (*shared_msg) = std::forward<T>(msg);
      stream_engine_
          ->broadcast<typename std::decay<decltype(msg)>::type, NoData>(
              protocol, std::move(shared_msg), boost::none);
    }

    template <typename T, typename H>
    void broadcast(const libp2p::peer::Protocol &protocol,
                   T &&msg,
                   H &&handshake) {
      auto shared_msg = KAGOME_EXTRACT_SHARED_CACHE(
          stream_engine, typename std::decay<decltype(msg)>::type);
      (*shared_msg) = std::forward<T>(msg);

      auto shared_handshake = KAGOME_EXTRACT_SHARED_CACHE(
          stream_engine, typename std::decay<decltype(handshake)>::type);
      (*shared_handshake) = std::forward<H>(handshake);

      stream_engine_->broadcast<typename std::decay_t<decltype(msg)>, NoData>(
          protocol, std::move(shared_msg), std::move(shared_handshake));
    }

    common::Logger logger_;
    StreamEngine::StreamEnginePtr stream_engine_;
    std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
        extrinsic_events_engine_;
    std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
        ext_event_key_repo_;
    std::shared_ptr<application::ChainSpec> config_;
    libp2p::peer::Protocol transactions_protocol_;
    libp2p::peer::Protocol block_announces_protocol_;
    boost::optional<libp2p::peer::PeerInfo> self_info_;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_BROADCAST_HPP
