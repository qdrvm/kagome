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

    enum struct ReservedStreamSetId { kLoopback = 1, kRemote };

   public:
    explicit GossiperBroadcast(libp2p::Host &host);

    ~GossiperBroadcast() override = default;

    void reserveStream(
        const libp2p::peer::PeerInfo &peer_info,
        std::shared_ptr<libp2p::connection::Stream> stream) override;

    void transactionAnnounce(const TransactionAnnounce &announce) override;

    void blockAnnounce(const BlockAnnounce &announce) override;

    void vote(const network::GrandpaVoteMessage &msg) override;

    void finalize(const network::GrandpaPreCommit &msg) override;

    void catchUpRequest(const libp2p::peer::PeerId &peer_id,
                        const CatchUpRequest &catch_up_request) override;

    void catchUpResponse(const libp2p::peer::PeerId &peer_id,
                         const CatchUpResponse &catch_up_response) override;

    outcome::result<void> addStream(std::shared_ptr<libp2p::connection::Stream> stream) override;

    uint32_t getActiveStreamNumber() override;

   private:
    using SubscriptionEngine = subscription::SubscriptionEngine<
        libp2p::peer::Protocol,
        std::shared_ptr<libp2p::connection::Stream> >;

    using SubscriptionEnginePtr = std::shared_ptr<SubscriptionEngine>;
    using SubscriberType = SubscriptionEngine::SubscriberType;
    using SubscriberPtr = std::shared_ptr<SubscriberType>;
    using PeerInfo = libp2p::peer::PeerInfo;
    using PeerId = libp2p::peer::PeerId;

    PeerInfo from(PeerId &pid) const {
      return PeerInfo {
          .id = pid,
          .addresses = {}
      };
    }

    PeerInfo from(PeerId &&pid) const {
      return PeerInfo {
          .id = std::move(pid),
          .addresses = {}
      };
    }

    outcome::result<PeerInfo> from(std::shared_ptr<libp2p::connection::Stream> &stream) const {
      BOOST_ASSERT(stream);
      auto peer_id_res = stream->remotePeerId();
      if (!peer_id_res.has_value()) {
        logger_->error("Can't get peer_id: {}", peer_id_res.error().message());
        return peer_id_res.error();
      }
      return from(std::move(peer_id_res.value()));
    }

    void uploadStream(SubscriberPtr &dst, std::shared_ptr<libp2p::connection::Stream> &stream) const {
      BOOST_ASSERT(dst && stream);
      dst->get()->reset();
      std::atomic_store(&dst->get(), stream);
    }

    template<typename T>
    void send(std::shared_ptr<libp2p::connection::Stream> stream, T &&msg) {
      ScaleMessageReadWriter read_writer(std::move(stream));
      read_writer.write(msg, [this](auto &&res) {
        if (!res) {
          logger_->error("Could not send message, reason: {}",
                         res.error().message());
        }
      });
    }

    void send(const libp2p::peer::PeerId &peer_id, GossipMessage &&msg);
    void broadcast(GossipMessage &&msg);

    libp2p::Host &host_;

    SubscriptionEnginePtr reserved_streams_engine_;
    SubscriptionEnginePtr syncing_streams_engine_;

    using ProtocolMap = std::unordered_map<libp2p::peer::Protocol, SubscriberPtr>;
    using PeerMap = std::unordered_map<PeerInfo, ProtocolMap>;
    PeerMap reserved_streams_;
    PeerMap syncing_streams_;

    common::Logger logger_;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_BROADCAST_HPP
