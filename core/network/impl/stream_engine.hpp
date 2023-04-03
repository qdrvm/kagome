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
#include <random>
#include <unordered_map>

#include <backward.hpp>

#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "log/logger.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/protocol_base.hpp"
#include "network/reputation_repository.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"
#include "utils/safe_object.hpp"

namespace kagome::network {

  template <typename Rng = std::mt19937>
  struct RandomGossipStrategy {
    RandomGossipStrategy(const size_t candidates_num,
                         const size_t lucky_peers_num)
        : candidates_num_{candidates_num} {
      auto lucky_rate = lucky_peers_num > 0
                          ? static_cast<double>(lucky_peers_num)
                                / std::max(candidates_num_, lucky_peers_num)
                          : 1.;
      threshold_ = gen_.max() * lucky_rate;
    }
    bool operator()(const PeerId &) {
      auto res = candidates_num_ > 0 && gen_() <= threshold_;
      return res;
    }

   private:
    Rng gen_;
    size_t candidates_num_;
    typename Rng::result_type threshold_;
  };

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

    template <typename... Args>
    static StreamEnginePtr create(Args &&...args) {
      return std::make_shared<StreamEngine>(std::forward<Args>(args)...);
    }

    outcome::result<void> add(std::shared_ptr<Stream> stream,
                              const std::shared_ptr<ProtocolBase> &protocol,
                              Direction direction);

    outcome::result<void> addIncoming(
        std::shared_ptr<Stream> stream,
        const std::shared_ptr<ProtocolBase> &protocol) {
      return add(std::move(stream), protocol, Direction::INCOMING);
    }

    outcome::result<void> addOutgoing(
        std::shared_ptr<Stream> stream,
        const std::shared_ptr<ProtocolBase> &protocol) {
      if (auto res = stream->remotePeerId(); res.has_value()) {
        SL_TRACE(logger_,
                 "Add outgoing protocol.(protocol={}, peer_id={})",
                 protocol->protocolName(),
                 res.value());
      }
      return add(std::move(stream), protocol, Direction::OUTGOING);
    }

    outcome::result<void> addBidirectional(
        std::shared_ptr<Stream> stream,
        const std::shared_ptr<ProtocolBase> &protocol) {
      return add(std::move(stream), protocol, Direction::BIDIRECTIONAL);
    }

    void reserveStreams(const PeerId &peer_id,
                        const std::shared_ptr<ProtocolBase> &protocol);

    void del(const PeerId &peer_id);

    bool reserveOutgoing(PeerId const &peer_id,
                         std::shared_ptr<ProtocolBase> const &protocol);

    void dropReserveOutgoing(PeerId const &peer_id,
                             std::shared_ptr<ProtocolBase> const &protocol);

    bool isAlive(PeerId const &peer_id,
                 std::shared_ptr<ProtocolBase> const &protocol) const;

    template <typename T>
    void send(const PeerId &peer_id,
              const std::shared_ptr<ProtocolBase> &protocol,
              std::shared_ptr<T> msg) {
      BOOST_ASSERT(msg != nullptr);
      BOOST_ASSERT(protocol != nullptr);

      bool was_sent = false;
      streams_.sharedAccess([&](auto const &streams) {
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
        const std::function<bool(const PeerId &peer_id)> &predicate,
        const std::function<void(libp2p::connection::Stream &)> on_send =
            [](auto &) {}) {
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
              on_send(*descr.outgoing.stream);
            } else {
              SL_TRACE(
                  logger_,
                  "No active outgoing. Reopen outgoing stream.(protocol={}, "
                  "peer={})",
                  protocol->protocolName(),
                  peer_id);
              descr.deferred_messages.push_back([weak_self = weak_from_this(),
                                                 msg,
                                                 peer_id,
                                                 protocol,
                                                 on_send](auto stream) {
                if (auto self = weak_self.lock()) {
                  SL_TRACE(self->logger_,
                           "Send deffered messages.(protocol={}, peer={})",
                           protocol->protocolName(),
                           peer_id);
                  self->send(peer_id, protocol, stream, msg);
                  on_send(*stream);
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

    size_t outgoingStreamsNumber(const std::shared_ptr<ProtocolBase> &protocol);

    template <typename F>
    size_t count(F &&filter) const {
      return streams_.sharedAccess([&](auto const &streams) {
        size_t result = 0;
        for (auto const &stream : streams) {
          if (filter(stream.first)) {
            result += stream.second.size();
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

    outcome::result<PeerInfo> from(std::shared_ptr<Stream> &stream) const;

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
    struct ProtocolDescr {
      std::shared_ptr<ProtocolBase> protocol;
      log::Logger logger =
          log::createLogger("ProtoDescription", "stream_engine");

      struct {
        std::shared_ptr<Stream> stream;
      } incoming;

      struct {
        std::shared_ptr<Stream> stream;
        bool reserved = false;
      } outgoing;

      std::deque<std::function<void(std::shared_ptr<Stream>)>>
          deferred_messages;

      void bt() {
        backward::StackTrace trace;
        trace.load_here(128);

        backward::TraceResolver tr; tr.load_stacktrace(trace);
        for (size_t i = 0; i < trace.size(); ++i) {
          backward::ResolvedTrace resolved_trace = tr.resolve(trace[i]);
          std::cout << "#" << i
                    << " " << resolved_trace.object_filename
                    << " " << resolved_trace.object_function
                    << ":" << resolved_trace.source.line
                    << " [" << resolved_trace.addr << "]"
                    << std::endl;
        }
      }

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
        //bt();
        return true;
      }

      bool isOutgoingReserved() const {
        return outgoing.reserved;
      }

      /**
       * Drops the flag that outgoing stream establishing.
       */
      void dropReserved() {
        //BOOST_ASSERT(outgoing.reserved);
        outgoing.reserved = false;
        //bt();
      }

      /**
       * Returns if descriptor contains active incoming stream.
       */
      [[maybe_unused]] bool hasActiveIncoming() const {
        return incoming.stream and not incoming.stream->isClosed();
      }
    };

    using ProtocolMap =
        std::map<std::shared_ptr<ProtocolBase>, struct ProtocolDescr>;
    using PeerMap = std::map<PeerId, ProtocolMap>;

    void uploadStream(std::shared_ptr<Stream> &dst,
                      std::shared_ptr<Stream> const &src,
                      std::shared_ptr<ProtocolBase> const &protocol,
                      Direction direction);

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
    static void forPeerProtocol(PeerId const &peer_id,
                                PM &streams,
                                std::shared_ptr<ProtocolBase> const &protocol,
                                F &&f) {
      if (auto it = streams.find(peer_id); it != streams.end()) {
        forProtocol(it->second, protocol, [&](auto &descr) {
          std::forward<F>(f)(it->second, descr);
        });
      }
    }

    [[maybe_unused]] void dump(std::string_view msg);

    void openOutgoingStream(PeerId const &peer_id,
                            std::shared_ptr<ProtocolBase> const &protocol,
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

#endif  // KAGOME_STREAM_ENGINE_HPP
