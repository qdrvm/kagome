/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROTOCOLBASEIMPL
#define KAGOME_NETWORK_PROTOCOLBASEIMPL

#include "network/protocol_base.hpp"

#include <memory>
#include <optional>
#include <string>

#include <libp2p/host/host.hpp>
#include <libp2p/peer/stream_protocols.hpp>

#include "log/logger.hpp"
#include "network/helpers/stream_read_buffer.hpp"
#include "utils/box.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {
  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::ProtocolName;
  using Protocols = libp2p::StreamProtocols;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;
  using Host = libp2p::Host;
  using ProtocolName = std::string;

  class ProtocolBaseImpl final : NonCopyable, NonMovable {
   public:
    ProtocolBaseImpl() = delete;
    ~ProtocolBaseImpl() = default;

    ProtocolBaseImpl(ProtocolName name,
                     libp2p::Host &host,
                     Protocols protocols,
                     log::Logger logger)
        : name_(std::move(name)),
          host_{host},
          protocols_{std::move(protocols)},
          log_{std::move(logger)} {}

    template <typename T>
    bool start(std::weak_ptr<T> wptr) {
      host_.setProtocolHandler(
          protocols_,
          [log{logger()}, wp(std::move(wptr))](auto &&stream_and_proto) {
            network::streamReadBuffer(stream_and_proto);
            if (auto peer_id = stream_and_proto.stream->remotePeerId()) {
              SL_TRACE(log,
                       "Handled {} protocol stream from: {}",
                       stream_and_proto.protocol,
                       peer_id.value().toBase58());
              if (auto self = wp.lock()) {
                self->onIncomingStream(std::move(stream_and_proto.stream));
                return;
              }
            } else {
              log->warn("Handled {} protocol stream from unknown peer",
                        stream_and_proto.protocol);
            }
            stream_and_proto.stream->close(
                [stream{stream_and_proto.stream}](auto &&) {});
          });
      return true;
    }

    bool stop() {
      return true;
    }

    const ProtocolName &protocolName() const {
      return name_;
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
      stream->close([log{logger()}, wptr, stream](auto &&result) {
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
    const ProtocolName name_;
    Host &host_;
    const Protocols protocols_;
    log::Logger log_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROTOCOLBASEIMPL
