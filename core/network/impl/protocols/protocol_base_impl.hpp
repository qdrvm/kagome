/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROTOCOLBASEIMPL
#define KAGOME_NETWORK_PROTOCOLBASEIMPL

#include "network/protocol_base.hpp"

#include <memory>
#include <string>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "utils/box.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {
  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;
  using Host = libp2p::Host;
  using ProtocolName = std::string;

  class ProtocolBaseImpl final : NonCopyable, NonMovable {
   public:
    ProtocolBaseImpl() = delete;
    ~ProtocolBaseImpl() = default;

    ProtocolBaseImpl(libp2p::Host &host,
                     Protocol const &protocol,
                     std::string const &log_section)
        : host_(host),
          protocol_(protocol),
          log_(log::createLogger(log_section, "kagome_protocols")) {}

    template <typename T>
    bool start(std::weak_ptr<T> wptr) {
      host_.setProtocolHandler(
          protocol_, [log(log_), wp(std::move(wptr))](auto &&stream) {
            if (auto self = wp.lock()) {
              if (auto peer_id = stream->remotePeerId()) {
                SL_TRACE(log,
                         "Handled {} protocol stream from: {}",
                         self->protocol(),
                         peer_id.value().toBase58());
                self->onIncomingStream(std::forward<decltype(stream)>(stream));
                return;
              }
              log->warn("Handled {} protocol stream from unknown peer",
                        self->protocol());
              stream->reset();
            }
          });
      return true;
    }

    bool stop() {
      return true;
    }

    Protocol const &protocol() const {
      return protocol_;
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
                    self->protocol(),
                    stream->remotePeerId().value());

          } else {
            SL_VERBOSE(log,
                    "Stream {} with {} was closed.",
                    self->protocol(),
                    stream->remotePeerId().value());
          }
        }
      });
    }

   private:
    Host &host_;
    Protocol const protocol_;
    log::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROTOCOLBASEIMPL
