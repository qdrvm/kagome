/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/stream_engine.hpp"
#include "log/formatters/optional.hpp"

namespace kagome::network {

  void StreamEngine::add(std::shared_ptr<Stream> stream,
                         const std::shared_ptr<ProtocolBase> &protocol,
                         Direction direction) {
    BOOST_ASSERT(protocol != nullptr);
    BOOST_ASSERT(stream != nullptr);

    auto peer_id = stream->remotePeerId().value();
    auto dir = static_cast<uint8_t>(direction);
    const bool is_incoming =
        (dir & static_cast<uint8_t>(Direction::INCOMING)) != 0;
    const bool is_outgoing =
        (dir & static_cast<uint8_t>(Direction::OUTGOING)) != 0;

    SL_TRACE(logger_,
             "Add stream for peer.(peer={}, protocol={})",
             peer_id,
             protocol->protocolName());

    streams_.exclusiveAccess([&](PeerMap &streams) {
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
    });
  }

  void StreamEngine::reserveStreams(
      const PeerId &peer_id, const std::shared_ptr<ProtocolBase> &protocol) {
    BOOST_ASSERT(protocol != nullptr);
    const auto reserved = streams_.exclusiveAccess([&](auto &streams) {
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
    SL_TRACE(logger_, "Remove all streams from peer.(peer={})", peer_id);
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

  void StreamEngine::del(const PeerId &peer_id,
                         const std::shared_ptr<ProtocolBase> &protocol) {
    SL_TRACE(logger_,
             "Remove {} streams from peer.(peer={})",
             protocol->protocolName(),
             peer_id);
    streams_.exclusiveAccess([&](auto &streams) {
      if (auto it = streams.find(peer_id); it != streams.end()) {
        auto &protocols = it->second;
        auto protocol_it = protocols.find(protocol);
        if (protocol_it != protocols.end()) {
          auto &descr = protocol_it->second;
          if (descr.incoming.stream) {
            descr.incoming.stream->reset();
          }
          if (descr.outgoing.stream) {
            descr.outgoing.stream->reset();
          }
          protocols.erase(protocol_it);
        }
        if (protocols.empty()) {
          streams.erase(it);
        }
      }
    });
  }

  bool StreamEngine::reserveOutgoing(
      const PeerId &peer_id, const std::shared_ptr<ProtocolBase> &protocol) {
    BOOST_ASSERT(protocol);
    return streams_.exclusiveAccess([&](PeerMap &streams) {
      auto &proto_map = streams[peer_id];
      auto [it, _] = proto_map.emplace(protocol, ProtocolDescr{protocol});
      return it->second.tryReserveOutgoing();
    });
  }

  void StreamEngine::dropReserveOutgoing(
      const PeerId &peer_id, const std::shared_ptr<ProtocolBase> &protocol) {
    BOOST_ASSERT(protocol);
    return streams_.exclusiveAccess([&](auto &streams) {
      forPeerProtocol(
          peer_id, streams, protocol, [&](auto, ProtocolDescr &descr) {
            return descr.dropReserved();
          });
    });
  }

  void StreamEngine::uploadStream(std::shared_ptr<Stream> &dst,
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

  void StreamEngine::openOutgoingStream(
      const PeerId &peer_id,
      const std::shared_ptr<ProtocolBase> &protocol,
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
