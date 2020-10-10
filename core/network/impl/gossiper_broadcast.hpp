/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_BROADCAST_HPP
#define KAGOME_GOSSIPER_BROADCAST_HPP

#include <gsl/span>
#include <unordered_map>

#include "common/logger.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "network/gossiper.hpp"
#include "network/types/gossip_message.hpp"
#include "network/types/peer_list.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"
#include "libp2p/peer/protocol.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/stream_engine.hpp"

namespace kagome::network {
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
    explicit GossiperBroadcast(libp2p::Host &host);

    ~GossiperBroadcast() override = default;

    void reserveStream(
        const libp2p::peer::PeerInfo &peer_info,
        const libp2p::peer::Protocol &protocol,
        std::shared_ptr<libp2p::connection::Stream> stream) override;

    void reserveLoopbackStream(
        const libp2p::peer::PeerInfo &info,
        const libp2p::peer::Protocol &protocol,
        std::shared_ptr<libp2p::connection::Stream> stream) override;

    void transactionAnnounce(const TransactionAnnounce &announce) override;

    void blockAnnounce(const BlockAnnounce &announce) override;

    void vote(const network::GrandpaVoteMessage &msg) override;

    void finalize(const network::GrandpaPreCommit &msg) override;

    void catchUpRequest(const libp2p::peer::PeerId &peer_id,
                        const CatchUpRequest &catch_up_request) override;

    void catchUpResponse(const libp2p::peer::PeerId &peer_id,
                         const CatchUpResponse &catch_up_response) override;

    outcome::result<void> addStream(
        const libp2p::peer::Protocol &protocol,
        std::shared_ptr<libp2p::connection::Stream> stream) override;

    uint32_t getActiveStreamNumber() override;

   private:
    void send(const libp2p::peer::PeerId &peer_id,
              const libp2p::peer::Protocol &protocol,
              GossipMessage &&msg);
    void broadcast(const libp2p::peer::Protocol &protocol, GossipMessage &&msg);

    common::Logger logger_;
    StreamEngine::StreamEnginePtr stream_engine_;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_BROADCAST_HPP
