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

    enum struct ReservedStreamSetId { kLoopback = 1, kRemote };

   public:
    explicit GossiperBroadcast(libp2p::Host &host);

    ~GossiperBroadcast() override = default;

    void reserveStream(
        const libp2p::peer::PeerInfo &peer_info,
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
    using SubscriptionEngine = subscription::SubscriptionEngine<
        libp2p::peer::Protocol,
        std::shared_ptr<libp2p::connection::Stream>>;

    using SubscriptionEnginePtr = std::shared_ptr<SubscriptionEngine>;
    using SubscriberType = SubscriptionEngine::SubscriberType;
    using SubscriberPtr = std::shared_ptr<SubscriberType>;
    using PeerInfo = libp2p::peer::PeerInfo;
    using PeerId = libp2p::peer::PeerId;
    using Protocol = libp2p::peer::Protocol;
    using ProtocolMap =
        std::unordered_map<libp2p::peer::Protocol, SubscriberPtr>;
    using PeerMap = std::unordered_map<PeerInfo, ProtocolMap>;

    PeerInfo from(PeerId &pid) const {
      return PeerInfo{.id = pid, .addresses = {}};
    }

    PeerInfo from(PeerId &&pid) const {
      return PeerInfo{.id = std::move(pid), .addresses = {}};
    }

    outcome::result<PeerInfo> from(
        std::shared_ptr<libp2p::connection::Stream> &stream) const {
      BOOST_ASSERT(stream);
      auto peer_id_res = stream->remotePeerId();
      if (!peer_id_res.has_value()) {
        logger_->error("Can't get peer_id: {}", peer_id_res.error().message());
        return peer_id_res.error();
      }
      return from(std::move(peer_id_res.value()));
    }

    void uploadStream(
        SubscriberPtr &dst,
        std::shared_ptr<libp2p::connection::Stream> &stream) const {
      BOOST_ASSERT(dst && stream);
      dst->get()->reset();
      std::atomic_store(&dst->get(), stream);
    }

    struct ProtocolDescriptor {
      std::reference_wrapper<ProtocolMap> proto_map;
      bool is_syncing;
    };

    boost::optional<ProtocolDescriptor> findPeer(const PeerInfo &peer) {
      auto find_if_exists = [&](auto &peer_map)
          -> boost::optional<std::reference_wrapper<ProtocolMap>> {
        if (auto it = peer_map.find(peer); it != peer_map.end())
          return std::reference_wrapper<ProtocolMap>(it->second);
        return boost::none;
      };

      if (auto proto_map = find_if_exists(syncing_streams_))
        return ProtocolDescriptor{.proto_map = std::move(*proto_map),
                                  .is_syncing = true};

      if (auto proto_map = find_if_exists(reserved_streams_))
        return ProtocolDescriptor{.proto_map = std::move(*proto_map),
                                  .is_syncing = false};

      return boost::none;
    }

    template <typename F>
    void forPeer(const PeerInfo &peer, F &&f) {
      if (auto proto_descriptor = findPeer(peer))
        std::forward<F>(f)(*proto_descriptor);
    }

    template <typename F>
    void forProtocol(ProtocolDescriptor proto_descriptor,
                     const Protocol &proto,
                     F &&f) {
      auto &proto_map = proto_descriptor.proto_map.get();
      if (auto it = proto_map.find(proto); it != proto_map.end()) {
        auto &sub = it->second;
        BOOST_ASSERT(sub && sub->get());
        if (proto_descriptor.is_syncing && sub->get()->isClosed())
          proto_map.erase(it);
        else
          std::forward<F>(f)(it->second);
      }
    }

    template <typename F>
    void forSubscriber(const PeerInfo &peer, const Protocol &proto, F &&f) {
      forPeer(peer, [&](auto &_) {
        forProtocol(_, proto, [&](auto &subscriber) {
          BOOST_ASSERT(subscriber && subscriber->get());
          std::forward<F>(f)(subscriber);
        });
      });
    }

    template <typename T>
    void send(std::shared_ptr<libp2p::connection::Stream> stream, T &&msg) {
      ScaleMessageReadWriter read_writer(std::move(stream));
      read_writer.write(msg, [this](auto &&res) {
        if (!res)
          logger_->error("Could not send message, reason: {}",
                         res.error().message());
      });
    }

    template <typename T>
    void send(const PeerInfo &peer, const Protocol &protocol, T &&msg) {
      forSubscriber(peer, protocol, [&](auto &subscriber) {
        BOOST_ASSERT(subscriber && subscriber->get());
        send(subscriber->get(), std::move(msg));
      });
    }

    void send(const libp2p::peer::PeerId &peer_id, GossipMessage &&msg);
    void broadcast(GossipMessage &&msg);

    libp2p::Host &host_;

    SubscriptionEnginePtr reserved_streams_engine_;
    SubscriptionEnginePtr syncing_streams_engine_;

    PeerMap reserved_streams_;
    PeerMap syncing_streams_;

    common::Logger logger_;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_BROADCAST_HPP
