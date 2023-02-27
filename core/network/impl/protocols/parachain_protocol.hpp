/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PARACHAINPROTOCOL
#define KAGOME_NETWORK_PARACHAINPROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/collation_observer.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/peer_view.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/collator_messages.hpp"
#include "network/types/roles.hpp"
#include "network/validation_observer.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  template <typename Observer, typename Message, bool kCollation>
  class ParachainProtocol final
      : public ProtocolBase,
        public std::enable_shared_from_this<
            ParachainProtocol<Observer, Message, kCollation>>,
        NonCopyable,
        NonMovable {
   public:
    using ObserverType = Observer;
    using MessageType = Message;

    ParachainProtocol() = delete;
    ~ParachainProtocol() override = default;

    ParachainProtocol(libp2p::Host &host,
                      application::AppConfiguration const &app_config,
                      application::ChainSpec const & /*chain_spec*/,
                      std::shared_ptr<ObserverType> observer,
                      Protocol const &protocol,
                      std::shared_ptr<network::PeerView> peer_view)
        : base_(kParachainProtocolName,
                host,
                {protocol},
                log::createLogger("ParachainProtocol", protocol)),
          observer_(std::move(observer)),
          app_config_{app_config},
          protocol_{protocol},
          peer_view_(std::move(peer_view)) {
      BOOST_ASSERT(peer_view_);
    }

    void onIncomingStream(std::shared_ptr<Stream> stream) override {
      BOOST_ASSERT(stream->remotePeerId().has_value());
      doCollatorHandshake<true>(
          stream,
          [wptr{this->weak_from_this()}](
              std::shared_ptr<Stream> const &stream) mutable {
            if (!stream) {
              return;
            }
            if (auto self = wptr.lock()) {
              self->readCollationMsg(stream);
            }
          });
    }

    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override {
      SL_DEBUG(base_.logger(),
               "Connect for {} stream with {}",
               protocolName(),
               peer_info.id);

      base_.host().newStream(
          peer_info.id,
          base_.protocolIds(),
          [wp{this->weak_from_this()},
           peer_id{peer_info.id},
           cb{std::move(cb)}](auto &&stream_res) mutable {
            auto self = wp.lock();
            if (not self) {
              cb(ProtocolError::GONE);
              return;
            }

            if (not stream_res.has_value()) {
              SL_VERBOSE(self->base_.logger(),
                         "Can't create outgoing {} stream with {}: {}",
                         self->protocolName(),
                         peer_id,
                         stream_res.error().message());
              cb(stream_res.as_failure());
              return;
            }

            auto &stream = stream_res.value().stream;
            BOOST_ASSERT(stream->remotePeerId().has_value());
            self->template doCollatorHandshake<false>(
                stream,
                [wptr{wp}, cb{std::move(cb)}](
                    std::shared_ptr<Stream> const &stream) mutable {
                  if (!stream) {
                    cb(ProtocolError::HANDSHAKE_ERROR);
                  } else {
                    cb(stream);
                  }
                });
          });
    }

    bool start() override {
      return base_.start(this->weak_from_this());
    }

    bool stop() override {
      return base_.stop();
    }

    const std::string &protocolName() const override {
      return protocol_;
    }

   private:
    template <bool DirectionIncoming, typename F>
    void exchangeHandshake(
        std::shared_ptr<kagome::network::Stream> const &stream, F &&func) {
      auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
      if constexpr (DirectionIncoming) {
        read_writer->read<Roles>([wptr{this->weak_from_this()},
                                  func{std::forward<F>(func)},
                                  stream](auto &&result) mutable {
          auto self = wptr.lock();
          if (!result || !self) {
            return std::forward<F>(func)(std::move(result));
          }

          auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
          read_writer->write(
              self->app_config_.roles(),
              [func{std::forward<F>(func)}, stream](auto &&result) mutable {
                return std::forward<F>(func)(std::move(result));
              });
        });
      } else {
        read_writer->write(
            app_config_.roles(),
            [wptr{this->weak_from_this()}, func{std::forward<F>(func)}, stream](
                auto &&result) mutable {
              auto self = wptr.lock();
              if (!result || !self) {
                return std::forward<F>(func)(std::move(result));
              }

              auto read_writer =
                  std::make_shared<ScaleMessageReadWriter>(stream);
              read_writer->read<Roles>(
                  [func{std::forward<F>(func)}, stream](auto &&result) mutable {
                    return std::forward<F>(func)(std::move(result));
                  });
            });
      }
    }

    template <bool DirectionIncoming, typename F>
    void doCollatorHandshake(
        std::shared_ptr<kagome::network::Stream> const &stream, F &&func) {
      exchangeHandshake<DirectionIncoming>(
          stream,
          [func{std::forward<F>(func)}, stream, wptr{this->weak_from_this()}](
              auto &&result) mutable {
            if (auto self = wptr.lock()) {
              if (!result) {
                SL_WARN(self->base_.logger(),
                        "Handshake with {} failed with error {}",
                        stream->remotePeerId().value(),
                        result.error().message());
                self->base_.closeStream(wptr, stream);
                std::forward<F>(func)(nullptr);
                return;
              }
              if constexpr (kCollation == true) {
                self->observer_->onIncomingCollationStream(
                    stream->remotePeerId().value());
              } else {
                self->observer_->onIncomingValidationStream(
                    stream->remotePeerId().value());
              }
            }
            std::forward<F>(func)(stream);
          });
    }

    void readCollationMsg(std::shared_ptr<kagome::network::Stream> stream) {
      auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
      read_writer->template read<WireMessage<MessageType>>(
          [wptr{this->weak_from_this()},
           stream{std::move(stream)}](auto &&result) mutable {
            auto self = wptr.lock();
            if (!self) {
              stream->close([](auto &&) {});
              return;
            }

            if (!result) {
              SL_WARN(
                  self->base_.logger(),
                  "Can't read incoming collation message from stream {} with "
                  "error {}",
                  stream->remotePeerId().value(),
                  result.error().message());
              self->base_.closeStream(wptr, stream);
              return;
            }

            SL_VERBOSE(self->base_.logger(),
                       "Received collation message from {}",
                       stream->remotePeerId().has_value()
                           ? stream->remotePeerId().value().toBase58()
                           : "{no peerId}");

            if (!stream->remotePeerId().has_value()) {
              SL_WARN(self->base_.logger(),
                      "Remote peer has no PeerId. Skip him.");
              return;
            }

            visit_in_place(
                std::move(result.value()),
                [&](ViewUpdate &&msg) {
                  SL_VERBOSE(self->base_.logger(),
                             "Received ViewUpdate from {}",
                             stream->remotePeerId().value().toBase58());
                  self->peer_view_->updateRemoteView(
                      stream->remotePeerId().value(), std::move(msg.view));
                },
                [&](MessageType &&p) {
                  SL_VERBOSE(self->base_.logger(),
                             "Received Collation/Validation message from {}",
                             stream->remotePeerId().value().toBase58());

                  self->observer_->onIncomingMessage(
                      stream->remotePeerId().value(), std::move(p));
                });
            self->readCollationMsg(std::move(stream));
          });
    }

    const static inline auto kParachainProtocolName = "ParachainProtocol"s;

    ProtocolBaseImpl base_;
    std::shared_ptr<ObserverType> observer_;
    application::AppConfiguration const &app_config_;
    Protocol const protocol_;
    std::shared_ptr<network::PeerView> peer_view_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PARACHAINPROTOCOL
