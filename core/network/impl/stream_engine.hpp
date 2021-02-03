/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_ENGINE_HPP
#define KAGOME_STREAM_ENGINE_HPP

#include <numeric>
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

  /**
   * Is the manager class to manipulate streams. It supports
   * [Peer] -> [Protocol_0] -> [Stream_0]
   *        -> [Protocol_1] -> [Stream_1]
   *        -> [   ....   ] -> [  ....  ]
   * relationships.
   */
  struct StreamEngine final : std::enable_shared_from_this<StreamEngine> {
    using PeerInfo = libp2p::peer::PeerInfo;
    using PeerId = libp2p::peer::PeerId;
    using Protocol = libp2p::peer::Protocol;
    using Stream = libp2p::connection::Stream;
    using Host = libp2p::Host;
    using StreamEnginePtr = std::shared_ptr<StreamEngine>;

   private:
    using ProtocolMap = std::unordered_map<Protocol, std::shared_ptr<Stream>>;
    using PeerMap = std::unordered_map<PeerId, ProtocolMap>;

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

    void add(const PeerId &peer_id, const Protocol &protocol) {
      std::unique_lock cs(streams_cs_);
      auto &protocols = streams_[peer_id];
      if (protocols.emplace(protocol, nullptr).second) {
        logger_->debug("Reserved stream (peer_id={}, protocol={})",
                       peer_id.toBase58(),
                       protocol);
      }
    }

    outcome::result<void> add(std::shared_ptr<Stream> stream,
                              const Protocol &protocol) {
      BOOST_ASSERT(!protocol.empty());
      BOOST_ASSERT(stream);

      OUTCOME_TRY(peer_id, stream->remotePeerId());
      bool existing = false;

      std::unique_lock cs(streams_cs_);
      forSubscriber(peer_id, protocol, [&](auto type, auto &subscriber) {
        existing = true;
        if (subscriber != stream) uploadStream(subscriber, std::move(stream));
      });
      if (existing) {
        return outcome::success();
      }

      auto &proto_map = streams_[peer_id];
      proto_map.emplace(protocol, std::move(stream));
      logger_->debug("Syncing stream (peer_id={}, protocol={})",
                     peer_id.toBase58(),
                     protocol);
      return outcome::success();
    }

    void del(const PeerId &peer_id, const Protocol &protocol) {
      std::unique_lock cs(streams_cs_);
      auto peer_it = streams_.find(peer_id);
      if (peer_it != streams_.end()) {
        auto &protocols = peer_it->second;
        auto protocol_it = protocols.find(protocol);
        if (protocol_it != protocols.end()) {
          protocols.erase(protocol_it);
          if (protocols.empty()) {
            streams_.erase(peer_it);
          }
        }
      }
    }

    void del(const PeerId &peer_id) {
      std::unique_lock cs(streams_cs_);
      auto it = streams_.find(peer_id);
      if (it != streams_.end()) {
        auto &protocols = it->second;
        for (auto &protocol : protocols) {
          if (protocol.second) {
            protocol.second->reset();
          }
        }
        streams_.erase(it);
      }
    }

    bool isAlive(const PeerId &peer_id, const Protocol &protocol) {
      std::unique_lock cs(streams_cs_);
      auto peer_it = streams_.find(peer_id);
      if (peer_it != streams_.end()) {
        auto &protocols = peer_it->second;
        auto protocol_it = protocols.find(protocol);
        if (protocol_it != protocols.end()) {
          auto& stream = protocol_it->second;
          return stream && not stream->isClosed();
        }
      }
      return false;
    }

    template <typename T>
    void send(std::shared_ptr<Stream> stream, const T &msg) {
      BOOST_ASSERT(stream);

      auto read_writer =
          std::make_shared<ScaleMessageReadWriter>(std::move(stream));
      read_writer->write(msg, [wp = weak_from_this()](auto &&res) {
        if (not res) {
          if (auto self = wp.lock()) {
            self->logger_->error("Could not send message, reason: {}",
                                 res.error().message());
          }
        }
      });
    }

    template <typename T, typename H>
    void send(const PeerId &peer_id,
              const Protocol &protocol,
              std::shared_ptr<T> msg,
              boost::optional<std::shared_ptr<H>> handshake) {
      BOOST_ASSERT(msg);
      BOOST_ASSERT(not protocol.empty());

      std::shared_lock cs(streams_cs_);
      forSubscriber(peer_id, protocol, [&](auto type, auto &stream) {
        if (stream) {
          send(stream, *msg);
          return;
        }

        updateStream(peer_id, protocol, std::move(msg), handshake);
      });
    }

    template <typename T, typename H>
    void broadcast(const Protocol &protocol,
                   std::shared_ptr<T> msg,
                   boost::optional<std::shared_ptr<H>> handshake) {
      BOOST_ASSERT(msg);
      BOOST_ASSERT(not protocol.empty());

      std::shared_lock cs(streams_cs_);
      forEachPeer([&](const auto &peer_id, auto &proto_map) {
        forProtocol(proto_map, protocol, [&](auto stream) {
          if (stream) {
            send(std::move(stream), *msg);
            return;
          }
          updateStream(peer_id, protocol, msg, handshake);
        });
      });
    }

    template <typename F>
    size_t count(F &&filter) const {
      size_t result = 0;
      for (const auto &i : streams_) {
        if (filter(i.first)) {
          result += i.second.size();
        }
      }
      return result;
    }

   private:
    Host &host_;
    common::Logger logger_;

    std::shared_mutex streams_cs_;
    PeerMap streams_;

   public:
    template <typename TPeerId,
              typename = std::enable_if<std::is_same_v<PeerId, TPeerId>>>
    PeerInfo from(TPeerId &&peer_id) const {
      return PeerInfo{.id = std::forward<TPeerId>(peer_id), .addresses = {}};
    }

    outcome::result<PeerInfo> from(std::shared_ptr<Stream> &stream) const {
      BOOST_ASSERT(stream);
      auto peer_id_res = stream->remotePeerId();
      if (!peer_id_res.has_value()) {
        logger_->error("Can't get peer_id: {}", peer_id_res.error().message());
        return peer_id_res.as_failure();
      }
      return from(std::move(peer_id_res.value()));
    }

    void uploadStream(std::shared_ptr<Stream> &dst,
                      std::shared_ptr<Stream> src) {
      BOOST_ASSERT(src);
      // Skip the same stream
      if (dst.get() == src.get()) {
        return;
      }
      // Reset previous stream
      if (dst) {
        dst->reset();
      }
      dst = std::move(src);
      if (dst->remotePeerId().has_value()) {
        logger_->debug("Stream (peer_id={}) was stored",
                       dst->remotePeerId().value().toBase58());
      }
    }

    boost::optional<ProtocolMap> findPeer(const PeerId &peer_id) {
      auto find_if_exists = [&](auto &peer_map)
          -> boost::optional<std::reference_wrapper<ProtocolMap>> {
        if (auto it = peer_map.find(peer_id); it != peer_map.end()) {
          return std::ref(it->second);
        }
        return boost::none;
      };

      if (auto proto_map = find_if_exists(streams_)) {
        return proto_map.value().get();
      }
      return boost::none;
    }

    template <typename F>
    void forPeer(const PeerId &peer_id, F &&f) {
      if (auto proto_descriptor = findPeer(peer_id)) {
        std::forward<F>(f)(*proto_descriptor);
      }
    }

    template <typename F>
    void forEachPeer(F &&f) {
      for (auto &peer_map : streams_) {
        std::forward<F>(f)(peer_map.first, peer_map.second);
      }
    }

    template <typename F>
    void forProtocol(ProtocolMap &proto_map, const Protocol &proto, F &&f) {
      if (auto it = proto_map.find(proto); it != proto_map.end()) {
        auto &stream = it->second;
        std::forward<F>(f)(stream);
      }
    }

    template <typename F>
    void forSubscriber(const PeerId &peer_id, const Protocol &proto, F &&f) {
      forPeer(peer_id, [&](auto &proto_map) {
        forProtocol(proto_map, proto, [&](auto &subscriber) {
          std::forward<F>(f)(proto_map, subscriber);
        });
      });
    }

   private:
    template <typename F, typename H>
    void forNewStream(const PeerId &peer_id,
                      const Protocol &protocol,
                      boost::optional<std::shared_ptr<H>> handshake,
                      F &&f) {
      using CallbackResultType =
          outcome::result<std::shared_ptr<libp2p::connection::Stream>>;
      host_.newStream(
          PeerInfo{.id = peer_id, .addresses = {}},
          protocol,
          [peer_id, handshake{std::move(handshake)}, f{std::forward<F>(f)}](
              auto &&stream_res) mutable {
            if (!stream_res || !handshake) {
              std::forward<F>(f)(std::move(stream_res));
              return;
            }

            auto stream = std::move(stream_res.value());
            auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
            BOOST_ASSERT(*handshake);
            read_writer->write(
                **handshake,
                [read_writer, stream, f{std::forward<F>(f)}](
                    auto &&write_res) mutable {
                  if (!write_res) {
                    std::forward<F>(f)(CallbackResultType{write_res.error()});
                    return;
                  }

                  read_writer->template read<H>(
                      [stream, f{std::forward<F>(f)}](
                          /*outcome::result<H>*/ auto &&read_res) mutable {
                        if (!read_res) {
                          std::forward<F>(f)(
                              CallbackResultType{read_res.error()});
                          return;
                        }
                        std::forward<F>(f)(
                            CallbackResultType{std::move(stream)});
                      });
                });
          });
    }

    template <typename T, typename H>
    void updateStream(const PeerId &peer_id,
                      const Protocol &protocol,
                      std::shared_ptr<T> msg,
                      boost::optional<std::shared_ptr<H>> handshake) {
      forNewStream(
          peer_id,
          protocol,
          std::move(handshake),
          [wp = weak_from_this(), protocol, peer_id, msg = std::move(msg)](
              auto &&stream_res) mutable {
            if (auto self = wp.lock()) {
              if (!stream_res) {
                self->logger_->error("Could not send message to {} Error: {}",
                                     peer_id.toBase58(),
                                     stream_res.error().message());
                return;
              }

              std::unique_lock cs(self->streams_cs_);
              auto stream = std::move(stream_res.value());

              bool existing = false;
              self->forSubscriber(
                  peer_id, protocol, [&](auto, auto &subscriber) {
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
