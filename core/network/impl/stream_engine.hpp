/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <unordered_map>

#if defined(BACKWARD_HAS_BACKTRACE)
#include <backward.hpp>
#endif

#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "log/logger.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/protocol_base.hpp"
#include "network/reputation_repository.hpp"
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
    using Protocol = libp2p::peer::ProtocolName;
    using Stream = libp2p::connection::Stream;
    using StreamEnginePtr = std::shared_ptr<StreamEngine>;

    static constexpr auto kDownVoteByDisconnectionExpirationTimeout =
        std::chrono::seconds(30);

    enum class Direction : uint8_t {
      INCOMING = 1,
      OUTGOING = 2,
      BIDIRECTIONAL = 3
    };

   public:
    StreamEngine(const StreamEngine &) = delete;
    StreamEngine &operator=(const StreamEngine &) = delete;

    StreamEngine(StreamEngine &&) = delete;
    StreamEngine &operator=(StreamEngine &&) = delete;

    ~StreamEngine() = default;
    explicit StreamEngine(
        std::shared_ptr<ReputationRepository> reputation_repository)
        : reputation_repository_(std::move(reputation_repository)),
          logger_{log::createLogger("StreamEngine", "network")} {}

    void add(std::shared_ptr<Stream> stream,
             const std::shared_ptr<ProtocolBase> &protocol,
             Direction direction);

    void addIncoming(std::shared_ptr<Stream> stream,
                     const std::shared_ptr<ProtocolBase> &protocol) {
      add(std::move(stream), protocol, Direction::INCOMING);
    }

    void addOutgoing(std::shared_ptr<Stream> stream,
                     const std::shared_ptr<ProtocolBase> &protocol) {
      if (auto res = stream->remotePeerId(); res.has_value()) {
        SL_TRACE(logger_,
                 "Add outgoing protocol.(protocol={}, peer_id={})",
                 protocol->protocolName(),
                 res.value());
      }
      add(std::move(stream), protocol, Direction::OUTGOING);
    }

    void addBidirectional(std::shared_ptr<Stream> stream,
                          const std::shared_ptr<ProtocolBase> &protocol) {
      add(std::move(stream), protocol, Direction::BIDIRECTIONAL);
    }

    void reserveStreams(const PeerId &peer_id,
                        const std::shared_ptr<ProtocolBase> &protocol);

    void del(const PeerId &peer_id);

    void del(const PeerId &peer_id,
             const std::shared_ptr<ProtocolBase> &protocol);

    bool reserveOutgoing(const PeerId &peer_id,
                         const std::shared_ptr<ProtocolBase> &protocol);

    void dropReserveOutgoing(const PeerId &peer_id,
                             const std::shared_ptr<ProtocolBase> &protocol);

    template <typename T>
    void send(const PeerId &peer_id,
              const std::shared_ptr<ProtocolBase> &protocol,
              std::shared_ptr<T> msg) {
      BOOST_ASSERT(msg != nullptr);
      BOOST_ASSERT(protocol != nullptr);

      bool was_sent = false;
      streams_.sharedAccess([&](const auto &streams) {
        forPeerProtocol(
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
          forProtocol(proto_map, protocol, [&](ProtocolDescr &descr) {
            SL_TRACE(logger_,
                     "Sending msg to peer.(protocol={}, peer={})",
                     protocol->protocolName(),
                     peer_id);
            if (descr.hasActiveOutgoing()) {
              SL_TRACE(
                  logger_,
                  "Has active outgoing. Direct send.(protocol={}, peer={})",
                  protocol->protocolName(),
                  peer_id);
              send(peer_id, protocol, descr.outgoing.stream, msg);
            } else {
              SL_TRACE(
                  logger_,
                  "No active outgoing. Reopen outgoing stream.(protocol={}, "
                  "peer={})",
                  protocol->protocolName(),
                  peer_id);
              descr.deferred_messages.push_back(
                  [weak_self = weak_from_this(), msg, peer_id, protocol](
                      auto stream) {
                    if (auto self = weak_self.lock()) {
                      SL_TRACE(self->logger_,
                               "Send deffered messages.(protocol={}, peer={})",
                               protocol->protocolName(),
                               peer_id);
                      self->send(peer_id, protocol, stream, msg);
                    }
                  });
              openOutgoingStream(peer_id, protocol, descr);
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
    void forEachPeer(F &&f) {
      streams_.exclusiveAccess([&](auto &streams) {
        for (auto &[peer_id, protocol_map] : streams) {
          std::forward<F>(f)(peer_id, protocol_map);
        }
      });
    }

    template <typename F>
    void forEachPeer(F &&f) const {
      streams_.sharedAccess([&](const auto &streams) {
        for (auto const &[peer_id, protocol_map] : streams) {
          std::forward<F>(f)(peer_id, protocol_map);
        }
      });
    }

    template <typename F>
    void forEachPeer(const std::shared_ptr<ProtocolBase> &protocol,
                     const F &f) const {
      forEachPeer([&](const PeerId &peer, const ProtocolMap &protocols) {
        if (protocols.find(protocol) != protocols.end()) {
          f(peer);
        }
      });
    }

   private:
    struct ProtocolDescr {
      std::shared_ptr<ProtocolBase> protocol;
      log::Logger logger = log::createLogger("ProtoDescription", "network");

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
      bool tryReserveOutgoing() {
        if (outgoing.reserved or hasActiveOutgoing()) {
          return false;
        }

        outgoing.reserved = true;
        // bt();
        return true;
      }

      bool isOutgoingReserved() const {
        return outgoing.reserved;
      }

      /**
       * Drops the flag that outgoing stream establishing.
       */
      void dropReserved() {
        // BOOST_ASSERT(outgoing.reserved);
        outgoing.reserved = false;
        // bt();
      }

      /**
       * Returns if descriptor contains active incoming stream.
       */
      [[maybe_unused]] bool hasActiveIncoming() const {
        return incoming.stream and not incoming.stream->isClosed();
      }
    };

    using ProtocolMap =
        std::unordered_map<std::shared_ptr<ProtocolBase>, struct ProtocolDescr>;
    using PeerMap = std::unordered_map<PeerId, ProtocolMap>;

    void uploadStream(std::shared_ptr<Stream> &dst,
                      const std::shared_ptr<Stream> &src,
                      const std::shared_ptr<ProtocolBase> &protocol,
                      Direction direction);

    template <typename T>
    void send(const PeerId &peer_id,
              const std::shared_ptr<ProtocolBase> &protocol,
              std::shared_ptr<Stream> stream,
              const std::shared_ptr<T> &msg) {
      BOOST_ASSERT(stream != nullptr);

      auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
      read_writer->write(
          *msg,
          [wp(weak_from_this()), peer_id, protocol, stream](auto &&res) {
            if (auto self = wp.lock()) {
              if (res.has_value()) {
                SL_TRACE(self->logger_,
                         "Message sent to {} stream with {}",
                         protocol->protocolName(),
                         peer_id);
              } else {
                SL_DEBUG(self->logger_,
                         "Could not send message to {} stream with {}: {}",
                         protocol->protocolName(),
                         peer_id,
                         res.error());
                stream->reset();
              }
            }
          });
    }

    template <typename PM, typename F>
    static void forProtocol(PM &proto_map,
                            const std::shared_ptr<ProtocolBase> &protocol,
                            F &&f) {
      if (auto it = proto_map.find(protocol); it != proto_map.end()) {
        auto &descr = it->second;
        std::forward<F>(f)(descr);
      }
    }

    template <typename PM, typename F>
    static void forPeerProtocol(const PeerId &peer_id,
                                PM &streams,
                                const std::shared_ptr<ProtocolBase> &protocol,
                                F &&f) {
      if (auto it = streams.find(peer_id); it != streams.end()) {
        forProtocol(it->second, protocol, [&](auto &descr) {
          std::forward<F>(f)(it->second, descr);
        });
      }
    }

    void openOutgoingStream(const PeerId &peer_id,
                            const std::shared_ptr<ProtocolBase> &protocol,
                            ProtocolDescr &descr);

    template <typename T>
    void updateStream(const PeerId &peer_id,
                      const std::shared_ptr<ProtocolBase> &protocol,
                      std::shared_ptr<T> msg) {
      streams_.exclusiveAccess([&](auto &streams) {
        forPeerProtocol(peer_id, streams, protocol, [&](auto, auto &descr) {
          descr.deferred_messages.push_back(
              [wp(weak_from_this()), peer_id, protocol, msg(std::move(msg))](
                  std::shared_ptr<Stream> stream) {
                if (auto self = wp.lock()) {
                  self->send(peer_id, protocol, stream, msg);
                }
              });
          openOutgoingStream(peer_id, protocol, descr);
        });
      });
    }

    std::shared_ptr<ReputationRepository> reputation_repository_;
    log::Logger logger_;

    SafeObject<PeerMap> streams_;
  };

}  // namespace kagome::network
