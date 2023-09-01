/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
          log_{std::move(logger)} {
      BOOST_ASSERT(!protocols_.empty());
    }

    template <typename T>
    bool start(std::weak_ptr<T> wp) {
      host_.setProtocolHandler(
          protocols_,
          [wp = std::move(wp), log = logger()](auto &&stream_and_proto) {
            if (auto self = wp.lock()) {
              BOOST_ASSERT(stream_and_proto.stream);

              network::streamReadBuffer(stream_and_proto);

              auto &[stream, protocol] = stream_and_proto;
              BOOST_ASSERT(stream);

              if (auto peer_id = stream->remotePeerId()) {
                SL_TRACE(log,
                         "Handled {} protocol stream from {}",
                         protocol,
                         peer_id);
                BOOST_ASSERT(stream);
                self->onIncomingStream(std::move(stream));
                return;
              }

              SL_WARN(log,
                      "Handled {} protocol stream from unknown peer",
                      protocol);
              BOOST_ASSERT(stream);
              stream->close([](auto &&) {});
            }
          });
      return true;
    }

    const ProtocolName &protocolName() const {
      return name_;
    }

    const Protocols &protocolIds() const {
      return protocols_;
    }

    Host &host() {
      return host_;
    }

    const log::Logger &logger() const {
      return log_;
    }

    template <typename T>
    void closeStream(std::weak_ptr<T> wp, std::shared_ptr<Stream> stream) {
      BOOST_ASSERT(stream);
      stream->close([wp = std::move(wp),
                     log = logger(),
                     peer_id = stream->remotePeerId().value()](auto &&result) {
        if (auto self = wp.lock()) {
          if (result.has_value()) {
            SL_DEBUG(log,
                     "Stream {} with {} was closed.",
                     self->protocolName(),
                     peer_id);
            return;
          }
          SL_DEBUG(log,
                   "Stream {} was not closed successfully with {}: {}",
                   self->protocolName(),
                   peer_id,
                   result.error());
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
