/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROTOCOLBASEIMPL
#define KAGOME_NETWORK_PROTOCOLBASEIMPL

#include "network/protocol_base.hpp"

#include <memory>
#include <string>

#include <libp2p/host/host.hpp>
#include <libp2p/peer/stream_protocols.hpp>

#include "utils/non_copyable.hpp"

namespace kagome::network {
  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using Protocols = libp2p::StreamProtocols;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;
  using Host = libp2p::Host;

  class ProtocolBaseImpl final : NonCopyable, NonMovable {
   public:
    ProtocolBaseImpl() = delete;
    ~ProtocolBaseImpl() = default;

    ProtocolBaseImpl(libp2p::Host &host,
                     Protocols const &protocols,
                     std::string const &log_section)
        : host_(host),
          protocols_{std::move(protocols)},
          log_(log::createLogger(log_section, "kagome_protocols")) {}

    template <typename T>
    bool start(std::weak_ptr<T> wptr) {
      host_.setProtocolHandler(
          protocols_,
          [log(log_), wp(std::move(wptr))](auto &&stream_and_proto) {
            if (auto self = wp.lock()) {
              if (auto peer_id = stream_and_proto.stream->remotePeerId()) {
                SL_TRACE(log,
                         "Handled {} protocol stream from: {}",
                         stream_and_proto.protocol,
                         peer_id.value().toBase58());
                self->onIncomingStream(
                    std::forward<decltype(stream_and_proto.stream)>(
                        stream_and_proto.stream));
                return;
              }
              log->warn("Handled {} protocol stream from unknown peer",
                        stream_and_proto.protocol);
              stream_and_proto.stream->reset();
            }
          });
      return true;
    }

    bool stop() {
      return true;
    }

    Protocols const &protocolIds() const {
      return protocols_;
    }

    Host &host() {
      return host_;
    }

    log::Logger const &logger() const {
      return log_;
    }

    template <typename T>
    void closeStream(std::weak_ptr<T> wptr, std::shared_ptr<Stream> stream) {
      BOOST_ASSERT(stream);
      stream->close([log(log_), wptr, stream](auto &&result) {
        if (auto self = wptr.lock()) {
          if (!result) {
            SL_WARN(log,
                    "Stream {} was not closed successfully with {}",
                    self->protocolName(),
                    stream->remotePeerId().value());

          } else {
            SL_VERBOSE(log,
                       "Stream {} with {} was closed.",
                       self->protocolName(),
                       stream->remotePeerId().value());
          }
        }
      });
    }

   private:
    Host &host_;
    Protocols const protocols_;
    log::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROTOCOLBASEIMPL
