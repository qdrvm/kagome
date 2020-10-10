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

  struct StreamEngine final : std::enable_shared_from_this<StreamEngine> {
    using PeerInfo = libp2p::peer::PeerInfo;
    using PeerId = libp2p::peer::PeerId;
    using Protocol = libp2p::peer::Protocol;
    using Stream = libp2p::connection::Stream;
    using Host = libp2p::Host;
    using StreamEnginePtr = std::shared_ptr<StreamEngine>;

    enum PeerType { kSyncing = 1, kReserved };
    enum ReservedStreamSetId { kLoopback = 1, kRemote };

   private:
    using SubscriptionEngine =
        subscription::SubscriptionEngine<Protocol, std::shared_ptr<Stream>>;
    using SubscriptionEnginePtr = std::shared_ptr<SubscriptionEngine>;
    using SubscriberType = SubscriptionEngine::SubscriberType;
    using SubscriberPtr = std::shared_ptr<SubscriberType>;
    using ProtocolMap = std::unordered_map<Protocol, SubscriberPtr>;
    using PeerMap = std::unordered_map<PeerInfo, ProtocolMap>;

    explicit StreamEngine(Host &host)
        : host_{host},
          reserved_streams_engine_{std::make_shared<SubscriptionEngine>()},
          syncing_streams_engine_{std::make_shared<SubscriptionEngine>()},
          logger_{common::createLogger("StreamEngine")} {}

   public:
    StreamEngine(const StreamEngine &) = delete;
    StreamEngine &operator=(const StreamEngine &) = delete;

    StreamEngine(StreamEngine &&) = default;
    StreamEngine &operator=(StreamEngine &&) = default;

    ~StreamEngine() = default;

    template <typename... Args>
    static StreamEnginePtr create(Args &&... args) {
      return std::make_shared<StreamEngine>(std::forward<Args>(args)...);
    }

    void addReserved(const PeerInfo &peer_info,
                     const Protocol &protocol,
                     ReservedStreamSetId stream_set_id,
                     std::shared_ptr<Stream> stream) {
      BOOST_ASSERT(!protocol.empty());

      auto &protocols = reserved_streams_[peer_info];
      BOOST_ASSERT(protocols.find(protocol) == protocols.end());

      auto subscriber =
          SubscriberType::create(reserved_streams_engine_, std::move(stream));
      subscriber->subscribe(stream_set_id, protocol);

      protocols.emplace(protocol, std::move(subscriber));
    }

    outcome::result<void> add(const Protocol &protocol,
                              std::shared_ptr<Stream> stream) {
      BOOST_ASSERT(stream);
      BOOST_ASSERT(!protocol.empty());

      OUTCOME_TRY(peer, from(stream));
      bool existing = false;

      forSubscriber(peer, protocol, [&](auto, auto &peer, auto &subscriber) {
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
      forSubscriber(
          peer, protocol, [&](auto type, auto &peer, auto &subscriber) {
            BOOST_ASSERT(subscriber);
            if (auto stream = subscriber->get()) {
              send(std::move(stream), std::move(msg));
              return;
            }

            BOOST_ASSERT(type == PeerType::kReserved);
            host_.newStream(peer,
                            protocol,
                            [wself{weak_from_this()},
                             subscriber,
                             peer,
                             msg{std::move(msg)}](auto &&stream_res) mutable {
                              if (auto self = wself.lock()) {
                                if (!stream_res) {
                                  self->logger_->error(
                                      "Could not send message to {} Error: {}",
                                      peer.id.toBase58(),
                                      stream_res.error().message());
                                  return;
                                }

                                auto stream = std::move(stream_res.value());
                                self->uploadStream(subscriber, stream);
                                self->send(stream, std::move(msg));
                              }
                            });
          });
    }

    uint32_t count() const {
      return syncing_streams_engine_->size() + reserved_streams_engine_->size();
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
      PeerType type;
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
                                  .type = PeerType::kSyncing};

      if (auto proto_map = find_if_exists(reserved_streams_))
        return ProtocolDescriptor{.proto_map = std::move(*proto_map),
                                  .type = PeerType::kReserved};

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
        BOOST_ASSERT(sub);
        if (proto_descriptor.type == PeerType::kSyncing
            && (!sub->get() || sub->get()->isClosed()))
          proto_map.erase(it);
        else
          std::forward<F>(f)(it->second);
      }
    }

    template <typename F>
    void forSubscriber(const PeerInfo &peer, const Protocol &proto, F &&f) {
      forPeer(peer, [&](auto &_) {
        forProtocol(_, proto, [&](auto &subscriber) {
          BOOST_ASSERT(subscriber);
          std::forward<F>(f)(_.type, peer, subscriber);
        });
      });
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_STREAM_ENGINE_HPP
