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
#include "libp2p/peer/protocol.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"

namespace kagome::network {

  struct StreamEngine final : std::enable_shared_from_this<StreamEngine> {
    using PeerInfo = libp2p::peer::PeerInfo;
    using PeerId = libp2p::peer::PeerId;
    using Protocol = libp2p::peer::Protocol;
    using Stream = libp2p::connection::Stream;
    using Host = libp2p::Host;
    using StreamEnginePtr = std::shared_ptr<StreamEngine>;

    enum PeerType { kSyncing = 1, kReserved };

   private:
    using ProtocolMap = std::unordered_map<Protocol, std::shared_ptr<Stream>>;
    using PeerMap = std::unordered_map<PeerInfo, ProtocolMap>;

    struct ProtocolDescriptor {
      std::reference_wrapper<ProtocolMap> proto_map;
      PeerType type;
    };

   public:
    StreamEngine(const StreamEngine &) = delete;
    StreamEngine &operator=(const StreamEngine &) = delete;

    StreamEngine(StreamEngine &&) = delete;
    StreamEngine &operator=(StreamEngine &&) = delete;

    ~StreamEngine() = default;
    explicit StreamEngine(Host &host)
        : host_{host}, logger_{common::createLogger("StreamEngine")} {}

    template <typename... Args>
    static StreamEnginePtr create(Args &&... args) {
      return std::make_shared<StreamEngine>(std::forward<Args>(args)...);
    }

    void addReserved(const PeerInfo &peer_info,
                     const Protocol &protocol,
                     std::shared_ptr<Stream> stream) {
      BOOST_ASSERT(!protocol.empty());

      std::unique_lock cs(streams_cs_);
      auto &protocols = reserved_streams_[peer_info];
      BOOST_ASSERT(protocols.find(protocol) == protocols.end());
      protocols.emplace(protocol, std::move(stream));
    }

    outcome::result<void> add(const Protocol &protocol,
                              std::shared_ptr<Stream> stream) {
      BOOST_ASSERT(stream);
      BOOST_ASSERT(!protocol.empty());

      OUTCOME_TRY(peer, from(stream));
      bool existing = false;

      std::unique_lock cs(streams_cs_);
      forSubscriber(peer, protocol, [&](auto type, auto &subscriber) {
        existing = true;
        if (subscriber != stream) uploadStream(subscriber, std::move(stream));
      });
      if (existing) return outcome::success();

      auto &proto_map = syncing_streams_[peer];
      proto_map.emplace(protocol, std::move(stream));
      logger_->debug("Syncing stream (peer_id={}) was emplaced",
                     peer.id.toHex());
      return outcome::success();
    }

    template <typename T>
    void send(std::shared_ptr<Stream> stream, const T &msg) {
      BOOST_ASSERT(stream);

      auto read_writer =
          std::make_shared<ScaleMessageReadWriter>(std::move(stream));
      read_writer->write(msg, [self{shared_from_this()}](auto &&res) {
        if (self && !res)
          self->logger_->error("Could not send message, reason: {}",
                               res.error().message());
      });
    }

    template <typename T>
    void send(const PeerInfo &peer,
              const Protocol &protocol,
              std::shared_ptr<T> msg) {
      BOOST_ASSERT(msg);
      BOOST_ASSERT(!protocol.empty());

      std::shared_lock cs(streams_cs_);
      forSubscriber(peer, protocol, [&](auto type, auto &stream) {
        if (stream) {
          send(stream, *msg);
          return;
        }

        BOOST_ASSERT(type == PeerType::kReserved);
        updateStream(peer, protocol, std::move(msg));
      });
    }

    template <typename T>
    void broadcast(const Protocol &protocol, std::shared_ptr<T> msg) {
      BOOST_ASSERT(msg);
      BOOST_ASSERT(!protocol.empty());

      std::shared_lock cs(streams_cs_);
      forEachPeer([&](const auto &peer, auto type, auto &proto_map) {
        forProtocol(ProtocolDescriptor{.proto_map = proto_map, .type = type},
                    protocol,
                    [&](auto stream) {
                      BOOST_ASSERT(type == PeerType::kReserved || stream);
                      if (stream) {
                        send(std::move(stream), *msg);
                        return;
                      }
                      BOOST_ASSERT(type == PeerType::kReserved);
                      updateStream(peer, protocol, msg);
                    });
      });
    }

    template <typename F>
    uint32_t count(F &&filter) const {
      uint32_t c = 0;
      auto enumerate = [&](const PeerMap &pm) {
        for (const auto &i : pm) {
          if (filter(i.first)) c += i.second.size();
        }
      };
      enumerate(syncing_streams_);
      enumerate(reserved_streams_);
      return c;
    }

   private:
    Host &host_;
    common::Logger logger_;

    std::shared_mutex streams_cs_;
    PeerMap reserved_streams_;
    PeerMap syncing_streams_;

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

    void uploadStream(std::shared_ptr<Stream> &dst,
                      std::shared_ptr<Stream> src) {
      BOOST_ASSERT(src);
      if (dst) dst->reset();
      dst = std::move(src);
      if (dst->remotePeerId().has_value())
        logger_->debug("Stream (peer_id={}) was stored",
                       dst->remotePeerId().value().toHex());
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
    void forEachPeer(F &&f) {
      for (auto &peer_map : syncing_streams_)
        std::forward<F>(f)(peer_map.first, PeerType::kSyncing, peer_map.second);

      for (auto &peer_map : reserved_streams_)
        std::forward<F>(f)(
            peer_map.first, PeerType::kReserved, peer_map.second);
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
            && (!sub || sub->isClosed()))
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
          std::forward<F>(f)(_.type, subscriber);
        });
      });
    }

    template <typename T>
    void updateStream(const PeerInfo &peer,
                      const Protocol &protocol,
                      std::shared_ptr<T> msg) {
      host_.newStream(
          peer,
          protocol,
          [wself{weak_from_this()}, protocol, peer, msg{std::move(msg)}](
              auto &&stream_res) mutable {
            if (auto self = wself.lock()) {
              if (!stream_res) {
                self->logger_->error("Could not send message to {} Error: {}",
                                     peer.id.toBase58(),
                                     stream_res.error().message());
                return;
              }

              std::unique_lock cs(self->streams_cs_);
              auto stream = std::move(stream_res.value());

              bool existing = false;
              self->forSubscriber(peer, protocol, [&](auto, auto &subscriber) {
                existing = true;
                self->uploadStream(subscriber, stream);
              });
              BOOST_ASSERT(existing);
              self->send(stream, *msg);
            }
          });
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_STREAM_ENGINE_HPP
