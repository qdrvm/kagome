/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <network/impl/stream_engine.hpp>

namespace kagome::network {

  outcome::result<void> StreamEngine::add(
      std::shared_ptr<Stream> stream,
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

    std::optional<kagome::network::StreamEngine::PeerId> p;
    if (auto r = stream->remotePeerId(); r.has_value()) {
      p = r.value();
    }
    SL_TRACE(logger_,
             "Add stream for peer.(peer={}, protocol={})",
             p,
             protocol->protocolName());

    return streams_.exclusiveAccess([&](auto &streams) {
      bool existing = false;
      forPeerProtocol(peer_id, streams, protocol, [&](auto type, auto &descr) {
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
        proto_map.emplace(protocol,
                          ProtocolDescr{protocol,
                                        is_incoming ? stream : nullptr,
                                        is_outgoing ? stream : nullptr});
        SL_DEBUG(logger_,
                 "Added {} {} stream with peer {}",
                 direction == Direction::INCOMING   ? "incoming"
                 : direction == Direction::OUTGOING ? "outgoing"
                                                    : "bidirectional",
                 protocol->protocolName(),
                 peer_id);
      }
      return outcome::success();
    });
  }

  void StreamEngine::reserveStreams(
      const PeerId &peer_id, const std::shared_ptr<ProtocolBase> &protocol) {
    BOOST_ASSERT(protocol != nullptr);
    auto const reserved = streams_.exclusiveAccess([&](auto &streams) {
      return streams[peer_id].emplace(protocol, ProtocolDescr{protocol}).second;
    });

    if (reserved) {
      SL_DEBUG(logger_,
               "Reserved {} stream with peer {}",
               protocol->protocolName(),
               peer_id);
    }
  }

  void StreamEngine::del(const PeerId &peer_id) {
    SL_TRACE(
        logger_, "Remove all streams from peer.(peer={})", peer_id.toBase58());
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

  bool StreamEngine::reserveOutgoing(
      PeerId const &peer_id, std::shared_ptr<ProtocolBase> const &protocol) {
    BOOST_ASSERT(protocol);
    return streams_.exclusiveAccess([&](PeerMap &streams) {
      auto &proto_map = streams[peer_id];
      auto [it, _] = proto_map.emplace(protocol, ProtocolDescr{protocol});
      return it->second.tryReserveOutgoing();
    });
  }

  void StreamEngine::dropReserveOutgoing(
      PeerId const &peer_id, std::shared_ptr<ProtocolBase> const &protocol) {
    BOOST_ASSERT(protocol);
    return streams_.exclusiveAccess([&](auto &streams) {
      forPeerProtocol(
          peer_id, streams, protocol, [&](auto, ProtocolDescr &descr) {
            return descr.dropReserved();
          });
    });
  }

  bool StreamEngine::isAlive(
      PeerId const &peer_id,
      std::shared_ptr<ProtocolBase> const &protocol) const {
    BOOST_ASSERT(protocol);
    bool alive = false;
    streams_.sharedAccess([&](auto const &streams) {
      forPeerProtocol(
          peer_id, streams, protocol, [&](auto, ProtocolDescr const &descr) {
            alive = descr.hasActiveOutgoing() || descr.hasActiveIncoming()
                 || descr.isOutgoingReserved();
          });
    });
    return alive;
  }

  size_t StreamEngine::outgoingStreamsNumber(
      const std::shared_ptr<ProtocolBase> &protocol) {
    size_t candidates_num{0};
    streams_.sharedAccess([&](auto const &streams) {
      candidates_num = std::count_if(
          streams.begin(), streams.end(), [&protocol](const auto &entry) {
            auto &[peer_id, protocol_map] = entry;
            return protocol_map.find(protocol) != protocol_map.end()
                && protocol_map.at(protocol).hasActiveOutgoing();
          });
    });
    return candidates_num;
  }

  outcome::result<PeerInfo> StreamEngine::from(
      std::shared_ptr<Stream> &stream) const {
    BOOST_ASSERT(stream);
    auto peer_id_res = stream->remotePeerId();
    if (!peer_id_res.has_value()) {
      logger_->error("Can't get peer_id: {}", peer_id_res.error());
      return peer_id_res.as_failure();
    }
    return from(std::move(peer_id_res.value()));
  }

  void StreamEngine::uploadStream(std::shared_ptr<Stream> &dst,
                                  std::shared_ptr<Stream> const &src,
                                  std::shared_ptr<ProtocolBase> const &protocol,
                                  Direction direction) {
    BOOST_ASSERT(src);
    // Skip the same stream
    if (dst.get() == src.get()) {
      return;
    }

    bool replaced = false;
    // Reset previous stream if any
    if (dst) {
      if (direction == Direction::INCOMING) {
        dst->close([](outcome::result<void>) {});
      } else {
        dst->reset();
      }
      replaced = true;
    }

    dst = src;
    SL_DEBUG(logger_,
             "{} {} stream with peer {} was {}",
             direction == Direction::BIDIRECTIONAL ? "Bidirectional"
             : direction == Direction::INCOMING    ? "Incoming"
                                                   : "Outgoing",
             protocol->protocolName(),
             dst->remotePeerId().has_value()
                 ? fmt::format("{}", dst->remotePeerId().value())
                 : "without PeerId",
             replaced ? "replaced" : "stored");
  }

  [[maybe_unused]] void StreamEngine::dump(std::string_view msg) {
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

  void StreamEngine::openOutgoingStream(
      PeerId const &peer_id,
      std::shared_ptr<ProtocolBase> const &protocol,
      ProtocolDescr &descr) {
    if (descr.tryReserveOutgoing()) {
      protocol->newOutgoingStream(
          PeerInfo{peer_id, {}},
          [wp(weak_from_this()), protocol, peer_id](auto &&stream_res) mutable {
            auto self = wp.lock();
            if (not self) {
              return;
            }

            if (!stream_res) {
              SL_DEBUG(self->logger_,
                       "Could not send message to new {} stream with {}: {}",
                       protocol->protocolName(),
                       peer_id,
                       stream_res.error());

              self->streams_.exclusiveAccess([&](auto &streams) {
                self->forPeerProtocol(
                    peer_id, streams, protocol, [&](auto, auto &descr) {
                      descr.deferred_messages.clear();
                      descr.dropReserved();
                    });
              });

              if (stream_res
                  == outcome::failure(
                      std::make_error_code(std::errc::not_connected))) {
                self->reputation_repository_->changeForATime(
                    peer_id,
                    reputation::cost::UNEXPECTED_DISCONNECT,
                    kDownVoteByDisconnectionExpirationTimeout);
              }

              return;
            }

            auto &stream = stream_res.value();
            self->streams_.exclusiveAccess([&](auto &streams) {
              [[maybe_unused]] bool existing = false;
              self->forPeerProtocol(
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
}  // namespace kagome::network
