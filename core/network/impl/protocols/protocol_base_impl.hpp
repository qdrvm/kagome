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

#include "utils/box.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {
  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using Protocols = libp2p::StreamProtocols;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;
  using Host = libp2p::Host;
  using ProtocolName = std::string;

  class ProtocolBaseImpl final : NonCopyable, NonMovable {
   public:
    ProtocolBaseImpl() = delete;
    ~ProtocolBaseImpl() = default;

    ProtocolBaseImpl(libp2p::Host &host,
                     Protocols const &protocols,
                     std::string const &log_section)
        : context_{std::make_shared<Context>(Context{
              .host_ = host,
              .protocols_ = std::move(protocols),
              .log_ = log::createLogger(log_section, "kagome_protocols"),
              .active_proto_ = std::nullopt})} {
      BOOST_ASSERT(context_ && !!"Context must be created!");
    }

    template <typename T>
    bool start(std::weak_ptr<T> wptr) {
      context_->host_.setProtocolHandler(
          context_->protocols_,
          [wcontext{std::weak_ptr<Context>(context_)},
           wp(std::move(wptr))](auto &&stream_and_proto) {
            if (auto context = wcontext.lock()) {
              context->active_proto_ = std::move(stream_and_proto.protocol);
              auto peer_id = stream_and_proto.stream->remotePeerId();

              if (peer_id) {
                SL_TRACE(context->log_,
                         "Handled {} protocol stream from: {}",
                         stream_and_proto.protocol,
                         peer_id.value().toBase58());
                if (auto self = wp.lock()) {
                  self->onIncomingStream(
                      std::forward<decltype(stream_and_proto.stream)>(
                          stream_and_proto.stream));
                  return;
                }
              } else {
                context->log_->warn(
                    "Handled {} protocol stream from unknown peer",
                    stream_and_proto.protocol);
              }
            }
            stream_and_proto.stream->close(
                [stream{stream_and_proto.stream}](auto &&) {});
          });
      return true;
    }

    bool stop() {
      return true;
    }

    Protocols const &protocolIds() const {
      return context_->protocols_;
    }

    std::optional<Protocol> const &protocol() const {
      return context_->active_proto_;
    }

    Host &host() {
      return context_->host_;
    }

    log::Logger const &logger() const {
      return context_->log_;
    }

    template <typename T>
    void closeStream(std::weak_ptr<T> wptr, std::shared_ptr<Stream> stream) {
      BOOST_ASSERT(stream);
      stream->close([log(context_->log_), wptr, stream](auto &&result) {
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
    struct Context {
      Host &host_;
      Protocols const protocols_;
      log::Logger log_;
      std::optional<Protocol> active_proto_;
    };

    std::shared_ptr<Context> context_;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROTOCOLBASEIMPL
