/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_ENGINE_HPP
#define KAGOME_STREAM_ENGINE_HPP

#include <deque>
#include <numeric>
#include <optional>
#include <queue>
#include <unordered_map>

#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "log/logger.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/protocol_base.hpp"
#include "network/rating_repository.hpp"
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

    static constexpr auto kDownVoteByDisconnectionExpirationTimeout =
        std::chrono::seconds(30);

    enum class Direction : uint8_t {
      INCOMING = 1,
      OUTGOING = 2,
      BIDIRECTIONAL = 3
    };

   private:
    struct ProtocolDescr {
      std::shared_ptr<ProtocolBase> protocol;

      struct {
        std::shared_ptr<Stream> stream;
      } incoming;

      struct {
        std::shared_ptr<Stream> stream;
        bool reserved = false;
      } outgoing;

      std::deque<std::function<void(std::shared_ptr<Stream>)>>
          deferred_messages;

     public:
      explicit ProtocolDescr(std::shared_ptr<ProtocolBase> proto)
          : protocol{std::move(proto)} {}
      ProtocolDescr(std::shared_ptr<ProtocolBase> proto,
                    std::shared_ptr<Stream> incoming_stream,
                    std::shared_ptr<Stream> outgoing_stream)
          : protocol{std::move(proto)},
            incoming{std::move(incoming_stream)},
            outgoing{std::move(outgoing_stream)} {}

      /**
       * Returns if descriptor contains active outgoing stream.
       */
      bool hasActiveOutgoing() const {
        return outgoing.stream and not outgoing.stream->isClosed();
      }

      /**
       * Sets the flag that outgoing stream establishing. To prevent the
       * situation of multiple streams to a single peer at the same time.
       */
      bool reserve() {
        if (outgoing.reserved or hasActiveOutgoing()) {
          return false;
        }

        outgoing.reserved = true;
        return true;
      }

      /**
       * Drops the flag that outgoing stream establishing.
       */
      void dropReserved() {
        BOOST_ASSERT(outgoing.reserved);
        outgoing.reserved = false;
      }

      /**
       * Returns if descriptor contains active incoming stream.
       */
      [[maybe_unused]] bool hasActiveIncoming() const {
        return incoming.stream and not incoming.stream->isClosed();
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
    StreamEngine(std::shared_ptr<PeerRatingRepository> peer_rating_repository)
        : peer_rating_repository_(std::move(peer_rating_repository)),
          logger_{log::createLogger("StreamEngine", "network")} {}

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
      const bool is_incoming =
          (dir & static_cast<uint8_t>(Direction::INCOMING)) != 0;
      const bool is_outgoing =
          (dir & static_cast<uint8_t>(Direction::OUTGOING)) != 0;

      return streams_.exclusiveAccess([&](auto &streams) {
        bool existing = false;
        forSubscriber(peer_id, streams, protocol, [&](auto type, auto &descr) {
          existing = true;
          if (is_incoming) {
            uploadStream(
                descr.incoming.stream, stream, protocol, Direction::INCOMING);
          }
          if (is_outgoing) {
            uploadStream(
                descr.outgoing.stream, stream, protocol, Direction::OUTGOING);
          }
        });

        if (not existing) {
          auto &proto_map = streams[peer_id];
          proto_map.emplace(protocol->protocol(),
                            ProtocolDescr{protocol,
                                          is_incoming ? stream : nullptr,
                                          is_outgoing ? stream : nullptr});
          SL_DEBUG(logger_,
                   "Added {} {} stream with peer {}",
                   direction == Direction::INCOMING   ? "incoming"
                   : direction == Direction::OUTGOING ? "outgoing"
                                                      : "bidirectional",
                   protocol->protocol(),
                   peer_id);
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

    outcome::result<void> addBidirectional(
        std::shared_ptr<Stream> stream,
        const std::shared_ptr<ProtocolBase> &protocol) {
      return add(std::move(stream), protocol, Direction::BIDIRECTIONAL);
    }

    void reserveStreams(const PeerId &peer_id,
                        const std::shared_ptr<ProtocolBase> &protocol) {
      BOOST_ASSERT(protocol != nullptr);
      auto const reserved = streams_.exclusiveAccess([&](auto &streams) {
        return streams[peer_id]
            .emplace(protocol->protocol(), ProtocolDescr{protocol})
            .second;
      });

      if (reserved) {
        SL_DEBUG(logger_,
                 "Reserved {} stream with peer {}",
                 protocol->protocol(),
                 peer_id);
      }
    }

    void del(const PeerId &peer_id) {
      streams_.exclusiveAccess([&](auto &streams) {
        if (auto it = streams.find(peer_id); it != streams.end()) {
          for (auto &protocol_it : it->second) {
            auto &descr = protocol_it.second;
            if (descr.incoming.stream) {
              descr.incoming.stream->reset();
            }
            if (descr.outgoing.stream) {
              descr.outgoing.stream->reset();
            }
          }
          streams.erase(it);
        }
      });
    }

    bool reserveOutgoing(PeerId const &peer_id,
                         std::shared_ptr<ProtocolBase> const &protocol) {
      BOOST_ASSERT(protocol);
      return streams_.exclusiveAccess([&](PeerMap &streams) {
        auto &proto_map = streams[peer_id];
        auto [it, _] =
            proto_map.emplace(protocol->protocol(), ProtocolDescr{protocol});
        return it->second.reserve();
      });
    }

    void dropReserveOutgoing(PeerId const &peer_id,
                             std::shared_ptr<ProtocolBase> const &protocol) {
      BOOST_ASSERT(protocol);
      return streams_.exclusiveAccess([&](auto &streams) {
        forSubscriber(peer_id, streams, protocol, [&](auto, auto &descr) {
          return descr.dropReserved();
        });
      });
    }

    bool isAlive(const PeerId &peer_id,
                 const std::shared_ptr<ProtocolBase> &protocol) {
      BOOST_ASSERT(protocol);
      return streams_.exclusiveAccess([&](auto &streams) {
        bool is_alive = false;
        forSubscriber(peer_id, streams, protocol, [&](auto, auto &descr) {
          if (descr.incoming.stream and not descr.incoming.stream->isClosed()) {
            is_alive = true;
          }
          if (descr.outgoing.stream and not descr.outgoing.stream->isClosed()) {
            is_alive = true;
          }
        });

        return is_alive;
      });
    };

    template <typename T>
    void send(const PeerId &peer_id,
              const std::shared_ptr<ProtocolBase> &protocol,
              std::shared_ptr<T> msg) {
      BOOST_ASSERT(msg != nullptr);
      BOOST_ASSERT(protocol != nullptr);

      bool was_sent = false;
      streams_.sharedAccess([&](auto const &streams) {
        forSubscriber(
            peer_id, streams, protocol, [&](auto type, auto const &descr) {
              if (descr.hasActiveOutgoing()) {
                send(peer_id, protocol, descr.outgoing.stream, msg);
                was_sent = true;
              }
            });
      });

      if (not was_sent) {
        updateStream(peer_id, protocol, msg);
      }
    }

    template <typename T>
    void broadcast(
        const std::shared_ptr<ProtocolBase> &protocol,
        const std::shared_ptr<T> &msg,
        const std::function<bool(const PeerId &peer_id)> &predicate) {
      BOOST_ASSERT(msg != nullptr);
      BOOST_ASSERT(protocol != nullptr);

      forEachPeer([&](const auto &peer_id, auto &proto_map) {
        if (predicate(peer_id)) {
          forProtocol(proto_map, protocol, [&](auto &descr) {
            if (descr.hasActiveOutgoing()) {
              send(peer_id, protocol, descr.outgoing.stream, msg);
            } else {
              updateStream(peer_id, protocol, descr);
            }
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
      return streams_.sharedAccess([&](auto const &streams) {
        size_t result = 0;
        for (auto const &i : streams) {
          if (filter(i.first)) {
            result += i.second.size();
          }
        }

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

    template <typename F>
    void forEachPeer(F &&f) {
      streams_.exclusiveAccess([&](auto &streams) {
        for (auto &[peer_id, protocol_map] : streams) {
          std::forward<F>(f)(peer_id, protocol_map);
        }
      });
    }

    template <typename F>
    void forEachPeer(F &&f) const {
      streams_.sharedAccess([&](auto const &streams) {
        for (auto const &[peer_id, protocol_map] : streams) {
          std::forward<F>(f)(peer_id, protocol_map);
        }
      });
    }

   private:
    void uploadStream(std::shared_ptr<Stream> &dst,
                      std::shared_ptr<Stream> const &src,
                      std::shared_ptr<ProtocolBase> const &protocol,
                      Direction direction) {
      BOOST_ASSERT(src);
      // Skip the same stream
      if (dst.get() == src.get()) return;

      bool replaced = false;
      // Reset previous stream if any
      if (dst) {
        dst->reset();
        replaced = true;
      }

      dst = src;
      SL_DEBUG(logger_,
               "{} {} stream with peer {} was {}",
               direction == Direction::BIDIRECTIONAL ? "Bidirectional"
               : direction == Direction::INCOMING    ? "Incoming"
                                                     : "Outgoing",
               protocol->protocol(),
               dst->remotePeerId().has_value()
                   ? fmt::format("{}", dst->remotePeerId().value())
                   : "without PeerId",
               replaced ? "replaced" : "stored");
    }

    template <typename T>
    void send(PeerId const &peer_id,
              std::shared_ptr<ProtocolBase> const &protocol,
              std::shared_ptr<Stream> stream,
              std::shared_ptr<T> const &msg) {
      BOOST_ASSERT(stream != nullptr);

      auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
      read_writer->write(
          *msg,
          [wp(weak_from_this()), peer_id, protocol, msg, stream](auto &&res) {
            if (auto self = wp.lock()) {
              if (res.has_value()) {
                SL_TRACE(self->logger_,
                         "Message sent to {} stream with {}",
                         protocol->protocol(),
                         peer_id);
              } else {
                SL_DEBUG(self->logger_,
                         "Could not send message to {} stream with {}: {}",
                         protocol->protocol(),
                         peer_id,
                         res.error().message());
                stream->reset();
              }
            }
          });
    }

    template <typename PM, typename F>
    static void forProtocol(PM &proto_map,
                            const std::shared_ptr<ProtocolBase> &protocol,
                            F &&f) {
      if (auto it = proto_map.find(protocol->protocol());
          it != proto_map.end()) {
        auto &descr = it->second;
        std::forward<F>(f)(descr);
      }
    }

    template <typename PM, typename F>
    static void forSubscriber(PeerId const &peer_id,
                              PM &streams,
                              std::shared_ptr<ProtocolBase> const &protocol,
                              F &&f) {
      if (auto it = streams.find(peer_id); it != streams.end()) {
        forProtocol(it->second, protocol, [&](auto &descr) {
          std::forward<F>(f)(it->second, descr);
        });
      }
    }

    [[maybe_unused]] void dump(std::string_view msg) {
      if (logger_->level() >= log::Level::DEBUG) {
        logger_->debug("DUMP: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
        logger_->debug("DUMP: {}", msg);
        forEachPeer([&](const auto &peer_id, auto const &proto_map) {
          logger_->debug("DUMP:   Peer {}", peer_id);
          for (auto const &[protocol, descr] : proto_map) {
            logger_->debug("DUMP:     Protocol {}", protocol);
            logger_->debug("DUMP:       I={} O={}   Messages:{}",
                           descr.incoming.stream,
                           descr.outgoing.stream,
                           descr.deferred_messages.size());
          }
        });
        logger_->debug("DUMP: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
      }
    }

    void updateStream(PeerId const &peer_id,
                      std::shared_ptr<ProtocolBase> const &protocol,
                      ProtocolDescr &descr) {
      if (descr.reserve()) {
        protocol->newOutgoingStream(
            PeerInfo{peer_id, {}},
            [wp(weak_from_this()), protocol, peer_id](
                auto &&stream_res) mutable {
              auto self = wp.lock();
              if (not self) {
                return;
              }

              if (!stream_res) {
                self->logger_->debug(
                    "Could not send message to new {} stream with {}: {}",
                    protocol->protocol(),
                    peer_id,
                    stream_res.error().message());

                if (stream_res
                    == outcome::failure(
                        std::make_error_code(std::errc::not_connected))) {
                  self->peer_rating_repository_->updateForATime(
                      peer_id,
                      -1000,
                      kDownVoteByDisconnectionExpirationTimeout);
                  return;
                }

                self->streams_.exclusiveAccess([&](auto &streams) {
                  self->forSubscriber(
                      peer_id, streams, protocol, [&](auto, auto &descr) {
                        descr.deferred_messages.clear();
                        descr.dropReserved();
                      });
                });

                return;
              }

              auto &stream = stream_res.value();
              self->streams_.exclusiveAccess([&](auto &streams) {
                [[maybe_unused]] bool existing = false;
                self->forSubscriber(
                    peer_id, streams, protocol, [&](auto, auto &descr) {
                      existing = true;
                      self->uploadStream(descr.outgoing.stream,
                                         stream,
                                         protocol,
                                         Direction::OUTGOING);
                      descr.dropReserved();

                      while (!descr.deferred_messages.empty()) {
                        auto &msg = descr.deferred_messages.front();
                        msg(stream);
                        descr.deferred_messages.pop_front();
                      }
                    });
                BOOST_ASSERT(existing);
              });
            });
      }
    }

    template <typename T>
    void updateStream(const PeerId &peer_id,
                      const std::shared_ptr<ProtocolBase> &protocol,
                      std::shared_ptr<T> msg) {
      streams_.exclusiveAccess([&](auto &streams) {
        forSubscriber(peer_id, streams, protocol, [&](auto, auto &descr) {
          descr.deferred_messages.push_back(
              [wp(weak_from_this()), peer_id, protocol, msg(std::move(msg))](
                  std::shared_ptr<Stream> stream) {
                if (auto self = wp.lock()) {
                  self->send(peer_id, protocol, stream, msg);
                }
              });
          updateStream(peer_id, protocol, descr);
        });
      });
    }

    std::shared_ptr<PeerRatingRepository> peer_rating_repository_;
    log::Logger logger_;

    SafeObject<PeerMap> streams_;
  };

}  // namespace kagome::network

#endif  // KAGOME_STREAM_ENGINE_HPP
