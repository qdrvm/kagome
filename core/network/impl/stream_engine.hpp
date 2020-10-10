/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_ENGINE_HPP
#define KAGOME_STREAM_ENGINE_HPP

#include <unordered_map>

#include "common/logger.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"
#include "libp2p/peer/protocol.hpp"
#include "network/helpers/scale_message_read_writer.hpp"

namespace kagome::network {

  struct StreamEngine final {
    using PeerInfo = libp2p::peer::PeerInfo;
    using PeerId = libp2p::peer::PeerId;
    using Protocol = libp2p::peer::Protocol;
    using Stream = libp2p::connection::Stream;
    using Host = libp2p::Host;

    enum struct ReservedStreamSetId { kLoopback = 1, kRemote };

   private:
    using SubscriptionEngine =
        subscription::SubscriptionEngine<Protocol, std::shared_ptr<Stream>>;
    using SubscriptionEnginePtr = std::shared_ptr<SubscriptionEngine>;
    using SubscriberType = SubscriptionEngine::SubscriberType;
    using SubscriberPtr = std::shared_ptr<SubscriberType>;
    using ProtocolMap = std::unordered_map<Protocol, SubscriberPtr>;
    using PeerMap = std::unordered_map<PeerInfo, ProtocolMap>;

   public:
    StreamEngine(const StreamEngine &) = delete;
    StreamEngine &operator=(const StreamEngine &) = delete;

    StreamEngine(StreamEngine &&) = default;
    StreamEngine &operator=(StreamEngine &&) = default;

    explicit StreamEngine(Host &host)
        : host_{host},
          reserved_streams_engine_{std::make_shared<SubscriptionEngine>()},
          syncing_streams_engine_{std::make_shared<SubscriptionEngine>()},
          logger_{common::createLogger("StreamEngine")} {}
    ~StreamEngine() = default;

    void addReserved(const PeerInfo &peer_info,
                     const Protocol &protocol,
                     std::shared_ptr<Stream> stream) {
      auto &protocols = reserved_streams_[peer_info];
      BOOST_ASSERT(protocols.find(protocol) == protocols.end());

      protocols.emplace(
          protocol,
          SubscriberType::create(reserved_streams_engine_, std::move(stream)));
    }

    outcome::result<void> addStream(const Protocol &protocol,
                                    std::shared_ptr<Stream> stream) {
      OUTCOME_TRY(peer, from(stream));

      bool existing = false;
      forSubscriber(peer, protocol, [&](auto &subscriber) {
        existing = true;
        if (subscriber->get() != stream) {
          uploadStream(subscriber, stream);
          logger_->debug("Stream (peer_id={}) was stored", peer.id.toHex());
        }
      });
      if (existing) return outcome::success();

      auto &proto_map = syncing_streams_[peer];
      proto_map.emplace(
          protocol,
          SubscriberType::create(syncing_streams_engine_, std::move(stream)));
      logger_->debug("Syncing stream (peer_id={}) was emplaced",
                     peer.id.toHex());
    }

    template <typename T>
    void send(std::shared_ptr<Stream> stream, T &&msg) {
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

   private:
    Host &host_;
    common::Logger logger_;

    SubscriptionEnginePtr reserved_streams_engine_;
    SubscriptionEnginePtr syncing_streams_engine_;
    PeerMap reserved_streams_;
    PeerMap syncing_streams_;

    struct ProtocolDescriptor {
      std::reference_wrapper<ProtocolMap> proto_map;
      bool is_syncing;
    };

    PeerInfo from(PeerId &pid) const {
      return PeerInfo{.id = pid, .addresses = {}};
    }

    PeerInfo from(PeerId &&pid) const {
      return PeerInfo{.id = std::move(pid), .addresses = {}};
    }

    outcome::result<PeerInfo> from(std::shared_ptr<Stream> &stream) const {
      BOOST_ASSERT(stream);
      auto peer_id_res = stream->remotePeerId();
      if (!peer_id_res.has_value()) {
        logger_->error("Can't get peer_id: {}", peer_id_res.error().message());
        return peer_id_res.error();
      }
      return from(std::move(peer_id_res.value()));
    }

    void uploadStream(SubscriberPtr &dst,
                      std::shared_ptr<Stream> &stream) const {
      BOOST_ASSERT(dst && stream);
      dst->get()->reset();
      std::atomic_store(&dst->get(), stream);
    }

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
  };

}  // namespace kagome::network

#endif  // KAGOME_STREAM_ENGINE_HPP
