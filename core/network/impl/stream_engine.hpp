/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_ENGINE_HPP
#define KAGOME_STREAM_ENGINE_HPP

#include <numeric>
#include <queue>
#include <unordered_map>
#include <optional>

#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "log/logger.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/protocol_base.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"
#include "utils/safe_object.hpp"

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
      std::optional<std::shared_ptr<Stream>> outgoing;
      std::queue<std::function<void(std::shared_ptr<Stream>)>>
          deffered_messages;

      bool hasActiveOutgoing() const {
        return outgoing && *outgoing && !((*outgoing)->isClosed());
      }

      bool hasActiveIncoming() const {
        return incoming && !incoming->isClosed();
      }
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
    static StreamEnginePtr create(Args &&...args) {
      return std::make_shared<StreamEngine>(std::forward<Args>(args)...);
    }

   private:
    outcome::result<void> add(std::shared_ptr<Stream> stream,
                              const std::shared_ptr<ProtocolBase> &protocol,
                              Direction direction) {
      BOOST_ASSERT(protocol != nullptr);
      BOOST_ASSERT(stream != nullptr);

      OUTCOME_TRY(peer_id, stream->remotePeerId());
      auto dir = static_cast<uint8_t>(direction);
      const bool is_incoming = dir & static_cast<uint8_t>(Direction::INCOMING);
      const bool is_outgoing = dir & static_cast<uint8_t>(Direction::OUTGOING);

      return streams_.exclusiveAccess([&](auto &streams) {
        bool existing = false;
        forSubscriber(peer_id, streams, protocol, [&](auto type, auto &subscriber) {
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
  
        if (!existing) {
          auto &proto_map = streams[peer_id];
          proto_map.emplace(protocol->protocol(),
                            ProtocolDescr{protocol,
                                          is_incoming ? stream : nullptr,
                                          is_outgoing ? stream : nullptr,
                                          {}});
          SL_DEBUG(logger_,
                   "Added {} {} stream with peer_id={}",
                   direction == Direction::INCOMING   ? "incoming"
                   : direction == Direction::OUTGOING ? "outgoing"
                                                      : "bidirectional",
                   protocol->protocol(),
                   peer_id.toBase58());
        }
        return outcome::success();
      });
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

    void reserve(const PeerId &peer_id,
             const std::shared_ptr<ProtocolBase> &protocol) {
      BOOST_ASSERT(protocol != nullptr);
      auto const reserved = streams_.exclusiveAccess([&](auto &streams) {
        return streams_[peer_id]
                .emplace(protocol->protocol(),
                         ProtocolDescr{protocol, {}, {}, {}})
                .second;
      });

      if (reserved) {
        SL_DEBUG(logger_,
                 "Reserved {} stream with {}",
                 protocol->protocol(),
                 peer_id.toBase58());
      }
    }

    outcome::result<void> add(std::shared_ptr<Stream> stream,
                              const std::shared_ptr<ProtocolBase> &protocol) {
      return add(std::move(stream), protocol, Direction::BIDIRECTIONAL);
    }

    void del(const PeerId &peer_id) {
      streams_.exclusiveAccess([&](auto &streams) {
        if (auto it = streams.find(peer_id); it != streams.end()) {
          for (auto &protocol_it : it->second) {
            auto &descr = protocol_it.second;
            if (descr.incoming)
              descr.incoming->reset();
            if (descr.outgoing)
              descr.outgoing->reset();
          }
          streams_.erase(it);
        }
      });
    }

    bool isAlive(const PeerId &peer_id,
                 const std::shared_ptr<ProtocolBase> &protocol) const {
      return streams_.sharedAccess([](auto const &streams) {
        auto peer_it = streams.find(peer_id);
        if (peer_it == streams.end()) {
          return false;
        }

        auto &protocols = peer_it->second;
        auto protocol_it = protocols.find(protocol->protocol());
        if (protocol_it == protocols.end()) {
          return false;
        }

        auto &descr = protocol_it->second;
        return (descr.incoming and not descr.incoming->isClosed())
               || (descr.outgoing and not descr.outgoing->isClosed());
      });
    }

    bool reserveOutgoing(PeerId const &peer_id,
                 std::shared_ptr<ProtocolBase> const &protocol) {
      BOOST_ASSERT(protocol);
      return streams_.exclusiveAccess([](auto &streams) {
        auto &descr = streams[peer_id][protocol->protocol()];
        if (descr.outgoing)
          return false;

        descr.outgoing = std::shared_ptr<Stream>{};
        return true;
      });
    }

    template <typename T>
    void send(const PeerId &peer_id,
              const std::shared_ptr<ProtocolBase> &protocol,
              std::shared_ptr<Stream> stream,
              const T &msg) {
      BOOST_ASSERT(stream != nullptr);

      auto read_writer =
          std::make_shared<ScaleMessageReadWriter>(std::move(stream));
      read_writer->write(
          msg, [wp(weak_from_this()), peer_id, protocol](auto &&res) {
            if (auto self = wp.lock()) {
              if (res.has_value()) {
                SL_TRACE(self->logger_,
                         "Message sent to {} stream with {}",
                         protocol->protocol(),
                         peer_id);
              } else {
                SL_ERROR(self->logger_,
                         "Could not send message to {} stream with {}: {}",
                         protocol->protocol(),
                         peer_id,
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

      streams_.sharedAccess([&](auto const &streams) {
        forSubscriber(peer_id, streams, protocol, [&](auto type, auto &descr) {
          if (descr.hasActiveOutgoing()) {
            send(peer_id, protocol, descr.outgoing, *msg);
            return;
          }
          updateStream(peer_id, protocol, std::move(msg));
        });
      });
    }

    template <typename T>
    void broadcast(
        const std::shared_ptr<ProtocolBase> &protocol,
        const std::shared_ptr<T> &msg,
        const std::function<bool(const PeerId &peer_id)> &predicate) {
      BOOST_ASSERT(msg != nullptr);
      BOOST_ASSERT(protocol != nullptr);

      forEachPeer([&](const auto &peer_id, auto &proto_map) {
        // check here under shared access
        if (predicate(peer_id)) {
          forProtocol(proto_map, protocol, [&](auto &descr) {
            if (descr.hasActiveOutgoing()) {
              send(peer_id, protocol, descr.outgoing, *msg);
              return;
            }
            updateStream(peer_id, protocol, msg);
          });
        }
      });
    }

    template <typename T>
    void broadcast(const std::shared_ptr<ProtocolBase> &protocol,
                   const std::shared_ptr<T> &msg) {
      static const std::function<bool(const PeerId &)> any =
          [](const PeerId &) { return true; };
      broadcast(protocol, msg, any);
    }

    template <typename F>
    size_t count(F &&filter) const {
      return streams_.sharedAccess([](auto const &streams) {
        size_t result = 0;
        for (auto const &i : streams)
          if (filter(i.first))
            result += i.second.size();

        return result;
      });
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
                      std::shared_ptr<Stream> const &src,
                      std::shared_ptr<ProtocolBase> const &protocol,
                      Direction direction) {
      BOOST_ASSERT(src);
      // Skip the same stream
      if (dst.get() == src.get())
        return;

      bool replaced = false;
      // Reset previous stream if any
      if (dst) {
        dst->reset();
        replaced = true;
      }

      dst = src;
      SL_DEBUG(logger_,
               "{} {} stream with peer_id={} was {}",
               direction == Direction::INCOMING ? "Incoming" : "Outgoing",
               protocol->protocol(),
               dst->remotePeerId().has_value() ? dst->remotePeerId().value().toBase58() : "{no PeerId}",
               replaced ? "replaced" : "stored");
    }

//    template <typename F>
//    void forEachPeer(F &&f) {
//      streams_.exclusiveAccess([&](auto &streams) {
//        for (auto &[peer_id, protocol_map] : streams_)
//          std::forward<F>(f)(peer_id, protocol_map);
//      });
//    }

    template <typename F>
    void forEachPeer(F &&f) const {
      streams_.sharedAccess([&](auto const &streams) {
        for (auto const &[peer_id, protocol_map] : streams_)
          std::forward<F>(f)(peer_id, protocol_map);
      });
    }

   private:
    static std::optional<std::reference_wrapper<ProtocolMap>> findPeer(
        const PeerId &peer_id, PeerMap const &streams) {
      auto find_if_exists = [&](auto &peer_map)
          -> std::optional<std::reference_wrapper<ProtocolMap>> {
        if (auto it = peer_map.find(peer_id); it != peer_map.end()) {
          return std::ref(it->second);
        }
        return std::nullopt;
      };

      if (auto proto_map = find_if_exists(streams)) {
        return proto_map.value();
      }
      return std::nullopt;
    }

    template <typename F>
    static void forPeer(const PeerId &peer_id, PeerMap const &streams, F &&f) {
      if (auto proto_descriptor = findPeer(peer_id, streams)) {
        std::forward<F>(f)(proto_descriptor->get());
      }
    }

    template <typename F>
    static void forProtocol(ProtocolMap &proto_map,
                     const std::shared_ptr<ProtocolBase> &protocol,
                     F &&f) {
      if (auto it = proto_map.find(protocol->protocol());
          it != proto_map.end()) {
        auto &descr = it->second;
        std::forward<F>(f)(descr);
      }
    }

    template <typename F>
    static void forSubscriber(PeerId const &peer_id,
                       PeerMap const &streams,
                       std::shared_ptr<ProtocolBase> const &protocol,
                       F &&f) {
      forPeer(peer_id, streams, [&](auto &proto_map) {
        forProtocol(proto_map, protocol, [&](auto &subscriber) {
          std::forward<F>(f)(proto_map, subscriber);
        });
      });
    }

    void dump(std::string_view msg) {
      if (logger_->level() >= log::Level::DEBUG) {
        logger_->debug("DUMP: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
        logger_->debug("DUMP: {}", msg);
        forEachPeer([&](const auto &peer_id, auto const &proto_map) {
          logger_->debug("DUMP:   Peer {}", peer_id.toBase58());
          for (auto const &[protocol, descr] : proto_map) {
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
            [wp = weak_from_this(), peer_id, protocol, msg = std::move(msg)](
                std::shared_ptr<Stream> stream) {
              if (auto self = wp.lock()) {
                self->send(peer_id, protocol, stream, *msg);
              }
            });
      });

      if (!need_to_create_new_stream)
        return;

      protocol->newOutgoingStream(
          PeerInfo{peer_id, {}},
          [wp = weak_from_this(), protocol, peer_id, msg = std::move(msg)](
              auto &&stream_res) mutable {
            auto self = wp.lock();
            if (not self) {
              return;
            }

            if (!stream_res) {
              self->logger_->error(
                  "Could not send message to {} stream with {}: {}",
                  protocol->protocol(),
                  peer_id,
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

            [[maybe_unused]] bool existing = false;
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
    SafeObject<PeerMap> streams_;
  };

}  // namespace kagome::network

#endif  // KAGOME_STREAM_ENGINE_HPP
