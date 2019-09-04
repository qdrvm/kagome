/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/stream_manager_libp2p.hpp"

namespace kagome::network {

  StreamManagerLibp2p::StreamManagerLibp2p(libp2p::Host &host) : host_{host} {}

  void StreamManagerLibp2p::submitStream(const PeerInfo &id,
                                         const Protocol &protocol,
                                         std::shared_ptr<Stream> stream) {
    auto peer_streams_it = streams_.find(id);
    if (peer_streams_it == streams_.end()) {
      streams_[id] = {{protocol, std::move(stream)}};
      return;
    }
    auto peer_streams = peer_streams_it->second;

    auto stream_it = peer_streams.find(protocol);
    if (stream_it != peer_streams.end()) {
      stream_it->second->reset();
    }

    peer_streams[protocol] = std::move(stream);
  }

  void StreamManagerLibp2p::getStream(
      const PeerInfo &id,
      const Protocol &protocol,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> cb) {
    auto peer_streams_it = streams_.find(id);
    if (peer_streams_it == streams_.end()) {
      return openStream(id, protocol, std::move(cb));
    }
    auto peer_streams = peer_streams_it->second;

    auto stream_it = peer_streams.find(protocol);
    if (stream_it == peer_streams.end()) {
      return openStream(id, protocol, std::move(cb));
    }

    return cb(stream_it->second);
  }

  void StreamManagerLibp2p::openStream(
      const PeerInfo &id,
      const Protocol &protocol,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> cb) {
    return host_.newStream(
        id,
        protocol,
        [self{shared_from_this()}, cb = std::move(cb), id, protocol](
            auto &&stream_res) {
          if (!stream_res) {
            return cb(stream_res.error());
          }
          auto stream = std::move(stream_res.value());

          self->submitStream(id, protocol, stream);
          cb(std::move(stream));
        });
  }

}  // namespace kagome::network
