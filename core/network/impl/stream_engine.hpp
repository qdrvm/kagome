/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_ENGINE_HPP
#define KAGOME_STREAM_ENGINE_HPP

#include <numeric>
#include <queue>
#include <unordered_map>

#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "log/logger.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/protocol_base.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"

namespace kagome::network {

  /**
   * Is the manager class to manipulate streams. It supports next structure
   * Peer
   *  ` ProtocolName_0
   *     ` ProtocolPtr_0,
   *       Incoming_Stream_0
   *       Outgoing_Stream_0
   *       MessagesQueue for creating outgoing stream
   */
  struct StreamEngine final : std::enable_shared_from_this<StreamEngine> {
    using PeerInfo = libp2p::peer::PeerInfo;
    using PeerId = libp2p::peer::PeerId;
    using Protocol = libp2p::peer::Protocol;
    using Stream = libp2p::connection::Stream;
    using StreamEnginePtr = std::shared_ptr<StreamEngine>;

    enum class Direction { INCOMING = 1, OUTGOING = 2, BIDIRECTIONAL = 3 };

   private:
    struct ProtocolDescr {
      std::shared_ptr<ProtocolBase> protocol;
      std::shared_ptr<Stream> incoming;
      std::shared_ptr<Stream> outgoing;
      std::queue<std::function<void(std::shared_ptr<Stream>)>>
          deffered_messages;
    };
    using ProtocolMap = std::map<Protocol, ProtocolDescr>;
    using PeerMap = std::map<PeerId, ProtocolMap>;

   public:
    StreamEngine(const StreamEngine &) = delete;
    StreamEngine &operator=(const StreamEngine &) = delete;

    StreamEngine(StreamEngine &&) = delete;
    StreamEngine &operator=(StreamEngine &&) = delete;

    ~StreamEngine() = default;
    explicit StreamEngine()
        : logger_{log::createLogger("StreamEngine", "network")} {}

    template <typename... Args>
    static StreamEnginePtr create(Args &&... args) {
      return std::make_shared<StreamEngine>(std::forward<Args>(args)...);
    }

    void add(const PeerId &peer_id,
             const std::shared_ptr<ProtocolBase> &protocol) {
      BOOST_ASSERT(protocol != nullptr);

      std::unique_lock cs(streams_cs_);
      auto &protocols = streams_[peer_id];
      if (protocols
              .emplace(protocol->protocol(),
                       ProtocolDescr{protocol, {}, {}, {}})
              .second) {
        SL_DEBUG(logger_,
                 "Reserved {} stream with {}",
                 protocol->protocol(),
                 peer_id.toBase58());
      }
    }

   private:
    outcome::result<void> add(std::shared_ptr<Stream> stream,
                              const std::shared_ptr<ProtocolBase> &protocol,
                              Direction direction) {
      BOOST_ASSERT(protocol != nullptr);
      BOOST_ASSERT(stream != nullptr);

      OUTCOME_TRY(peer_id, stream->remotePeerId());
      bool existing = false;
      auto dir = static_cast<uint8_t>(direction);
      const bool is_incoming = dir & static_cast<uint8_t>(Direction::INCOMING);
      const bool is_outgoing = dir & static_cast<uint8_t>(Direction::OUTGOING);

      std::unique_lock cs(streams_cs_);
      forSubscriber(peer_id, protocol, [&](auto type, auto &subscriber) {
        existing = true;
        if (is_incoming) {
          uploadStream(
              subscriber.incoming, stream, protocol, Direction::INCOMING);
        }
        if (is_outgoing) {
          uploadStream(
              subscriber.outgoing, stream, protocol, Direction::OUTGOING);
        }
      });
      if (existing) {
        return outcome::success();
      }

      auto &proto_map = streams_[peer_id];
      proto_map.emplace(protocol->protocol(),
                        ProtocolDescr{protocol,
                                      is_incoming ? stream : nullptr,
                                      is_outgoing ? stream : nullptr,
                                      {}});
      SL_DEBUG(
          logger_,
          "Added {} {} stream with peer_id={}",
          direction == Direction::INCOMING
              ? "incoming"
              : direction == Direction::OUTGOING ? "outgoing" : "bidirectional",
          protocol->protocol(),
          peer_id.toBase58());
      return outcome::success();
    }

   public:
    outcome::result<void> addIncoming(
        std::shared_ptr<Stream> stream,
        const std::shared_ptr<ProtocolBase> &protocol) {
      return add(std::move(stream), protocol, Direction::INCOMING);
    }

    outcome::result<void> addOutgoing(
        std::shared_ptr<Stream> stream,
        const std::shared_ptr<ProtocolBase> &protocol) {
      return add(std::move(stream), protocol, Direction::OUTGOING);
    }

    outcome::result<void> add(std::shared_ptr<Stream> stream,
                              const std::shared_ptr<ProtocolBase> &protocol) {
      return add(std::move(stream), protocol, Direction::BIDIRECTIONAL);
    }

    void del(const PeerId &peer_id,
             const std::shared_ptr<ProtocolBase> &protocol) {
      std::unique_lock cs(streams_cs_);
      auto peer_it = streams_.find(peer_id);
      if (peer_it != streams_.end()) {
        auto &protocols = peer_it->second;
        auto protocol_it = protocols.find(protocol->protocol());
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
        for (auto &protocol_it : protocols) {
          auto &descr = protocol_it.second;
          if (descr.incoming) {
            descr.incoming->reset();
          }
          if (descr.outgoing) {
            descr.outgoing->reset();
          }
        }
        streams_.erase(it);
      }
    }

    bool isAlive(const PeerId &peer_id,
                 const std::shared_ptr<ProtocolBase> &protocol) {
      std::unique_lock cs(streams_cs_);
      auto peer_it = streams_.find(peer_id);
      if (peer_it != streams_.end()) {
        auto &protocols = peer_it->second;
        auto protocol_it = protocols.find(protocol->protocol());
        if (protocol_it != protocols.end()) {
          auto &descr = protocol_it->second;
          if (descr.incoming and not descr.incoming->isClosed()) {
            return true;
          }
          if (descr.outgoing and not descr.outgoing->isClosed()) {
            return true;
          }
        }
      }
      return false;
    }

    template <typename T>
    void send(std::shared_ptr<Stream> stream, const T &msg) {
      BOOST_ASSERT(stream != nullptr);

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

    template <typename T>
    void send(const PeerId &peer_id,
              const std::shared_ptr<ProtocolBase> &protocol,
              std::shared_ptr<T> msg) {
      BOOST_ASSERT(msg != nullptr);
      BOOST_ASSERT(protocol != nullptr);

      std::shared_lock cs(streams_cs_);
      forSubscriber(peer_id, protocol, [&](auto type, auto &descr) {
        if (descr.outgoing and not descr.outgoing->isClosed()) {
          send(descr.outgoing, *msg);
          return;
        }

        updateStream(peer_id, protocol, std::move(msg));
      });
    }

    template <typename T>
    void broadcast(const std::shared_ptr<ProtocolBase> &protocol,
                   std::shared_ptr<T> msg) {
      BOOST_ASSERT(msg != nullptr);
      BOOST_ASSERT(protocol != nullptr);

      std::shared_lock cs(streams_cs_);
      forEachPeer([&](const auto &peer_id, auto &proto_map) {
        forProtocol(proto_map, protocol, [&](auto &descr) {
          if (descr.outgoing) {
            send(descr.outgoing, *msg);
            return;
          }
          updateStream(peer_id, protocol, msg);
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
                      const std::shared_ptr<Stream> &src,
                      const std::shared_ptr<ProtocolBase> &protocol,
                      Direction direction) {
      BOOST_ASSERT(src);
      // Skip the same stream
      if (dst.get() == src.get()) {
        return;
      }
      bool replaced = false;
      // Reset previous stream if any
      if (dst) {
        dst->reset();
        replaced = true;
      }
      dst = src;
      if (dst->remotePeerId().has_value()) {
        SL_DEBUG(logger_,
                 "{} {} stream with peer_id={} was {}",
                 direction == Direction::INCOMING ? "Incoming" : "Outgoing",
                 protocol->protocol(),
                 dst->remotePeerId().value().toBase58(),
                 replaced ? "replaced" : "stored");
      }
    }

    boost::optional<std::reference_wrapper<ProtocolMap>> findPeer(
        const PeerId &peer_id) {
      auto find_if_exists = [&](auto &peer_map)
          -> boost::optional<std::reference_wrapper<ProtocolMap>> {
        if (auto it = peer_map.find(peer_id); it != peer_map.end()) {
          return std::ref(it->second);
        }
        return boost::none;
      };

      if (auto proto_map = find_if_exists(streams_)) {
        return proto_map.value();
      }
      return boost::none;
    }

    template <typename F>
    void forPeer(const PeerId &peer_id, F &&f) {
      if (auto proto_descriptor = findPeer(peer_id)) {
        std::forward<F>(f)(proto_descriptor->get());
      }
    }

    template <typename F>
    void forEachPeer(F &&f) {
      for (auto &[peer_id, protocol_map] : streams_) {
        std::forward<F>(f)(peer_id, protocol_map);
      }
    }

    template <typename F>
    void forProtocol(ProtocolMap &proto_map,
                     const std::shared_ptr<ProtocolBase> &protocol,
                     F &&f) {
      if (auto it = proto_map.find(protocol->protocol());
          it != proto_map.end()) {
        auto &descr = it->second;
        std::forward<F>(f)(descr);
      }
    }

    template <typename F>
    void forSubscriber(const PeerId &peer_id,
                       const std::shared_ptr<ProtocolBase> &protocol,
                       F &&f) {
      forPeer(peer_id, [&](auto &proto_map) {
        forProtocol(proto_map, protocol, [&](auto &subscriber) {
          std::forward<F>(f)(proto_map, subscriber);
        });
      });
    }

   private:
    void dump(std::string_view msg) {
      if (logger_->level() >= log::Level::DEBUG) {
        logger_->debug("DUMP: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
        logger_->debug("DUMP: {}", msg);
        forEachPeer([&](const auto &peer_id, auto &proto_map) {
          logger_->debug("DUMP:   Peer {}", peer_id.toBase58());
          for (auto &[protocol, descr] : proto_map) {
            logger_->debug("DUMP:     Protocol {}", protocol);
            logger_->debug("DUMP:       I={} O={}   Messages:{}",
                           descr.incoming,
                           descr.outgoing,
                           descr.deffered_messages.size());
          }
        });
        logger_->debug("DUMP: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
      }
    }

    template <typename T>
    void updateStream(const PeerId &peer_id,
                      const std::shared_ptr<ProtocolBase> &protocol,
                      std::shared_ptr<T> msg) {
      bool need_to_create_new_stream = true;

      forSubscriber(peer_id, protocol, [&](auto, auto &subscriber) {
        need_to_create_new_stream = subscriber.deffered_messages.empty();
        subscriber.deffered_messages.push(
            [this, msg = std::move(msg)](std::shared_ptr<Stream> stream) {
              send(stream, *msg);
            });
      });

      if (not need_to_create_new_stream) {
        return;
      }

      protocol->newOutgoingStream(
          PeerInfo{peer_id, {}},
          [wp = weak_from_this(), protocol, peer_id, msg = std::move(msg)](
              auto &&stream_res) mutable {
            auto self = wp.lock();
            if (not self) {
              return;
            }

            if (!stream_res) {
              self->logger_->error("Could not send message to {}: Error: {}",
                                   peer_id.toBase58(),
                                   stream_res.error().message());
              self->forSubscriber(
                  peer_id, protocol, [&](auto, auto &subscriber) {
                    while (not subscriber.deffered_messages.empty()) {
                      subscriber.deffered_messages.pop();
                    }
                  });

              return;
            }
            auto &stream = stream_res.value();

            std::unique_lock cs(self->streams_cs_);

            bool existing = false;
            self->forSubscriber(peer_id, protocol, [&](auto, auto &subscriber) {
              existing = true;
              self->uploadStream(
                  subscriber.outgoing, stream, protocol, Direction::OUTGOING);
            });
            BOOST_ASSERT(existing);

            self->forSubscriber(peer_id, protocol, [&](auto, auto &subscriber) {
              while (not subscriber.deffered_messages.empty()) {
                auto &msg = subscriber.deffered_messages.front();
                msg(stream);
                subscriber.deffered_messages.pop();
              }
            });
          });
    }

    log::Logger logger_;
    std::shared_mutex streams_cs_;
    PeerMap streams_;
  };

}  // namespace kagome::network

#endif  // KAGOME_STREAM_ENGINE_HPP
