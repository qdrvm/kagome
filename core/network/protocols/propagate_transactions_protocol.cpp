/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/protocols/propagate_transactions_protocol.hpp"

#include "network/common.hpp"
#include "network/types/no_data_message.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network,
                            PropagateTransactionsProtocol::Error,
                            e) {
  using E = kagome::network::PropagateTransactionsProtocol::Error;
  switch (e) {
    case E::GONE:
      return "Protocol was switched off";
  }
  return "Unknown error";
}

namespace kagome::network {

  PropagateTransactionsProtocol::PropagateTransactionsProtocol(
      libp2p::Host &host,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<StreamEngine> stream_engine)
      : host_(host),
        extrinsic_observer_(std::move(extrinsic_observer)),
        stream_engine_(std::move(stream_engine)) {
    BOOST_ASSERT(extrinsic_observer_ != nullptr);
    BOOST_ASSERT(stream_engine_ != nullptr);
    const_cast<Protocol &>(protocol_) = fmt::format(
        kPropagateTransactionsProtocol.data(), chain_spec.protocolId());
  }

  bool PropagateTransactionsProtocol::start() {
    host_.setProtocolHandler(protocol_, [wp = weak_from_this()](auto &&stream) {
      if (auto self = wp.lock()) {
        if (auto peer_id = stream->remotePeerId()) {
          self->log_->trace("Handled {} protocol stream from: {}",
                            self->protocol_,
                            peer_id.value().toBase58());
          self->onIncomingStream(std::forward<decltype(stream)>(stream));
          return;
        }
        self->log_->warn("Handled {} protocol stream from unknown peer",
                         self->protocol_);
      }
    });
    return true;
  }

  bool PropagateTransactionsProtocol::stop() {
    return true;
  }

  void PropagateTransactionsProtocol::onIncomingStream(
      std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());

    readHandshake(
        stream,
        Direction::INCOMING,
        [wp = weak_from_this(),
         stream](outcome::result<std::shared_ptr<Stream>> stream_res) {
          auto self = wp.lock();
          if (not self) {
            return;
          }
          if (stream_res.has_value()) {
            std::ignore = self->stream_engine_->addIncoming(stream, self);
            self->log_->info("Fully established incoming {} stream with {}",
                             self->protocol_,
                             stream->remotePeerId().value().toBase58());
            return;
          }
          self->log_->info("Fail establishing incoming {} stream with {}: {}",
                           self->protocol_,
                           stream->remotePeerId().value().toBase58(),
                           stream_res.error().message());
        });
  }

  void PropagateTransactionsProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    host_.newStream(
        peer_info,
        protocol_,
        [wp = weak_from_this(), peer_id = peer_info.id, cb = std::move(cb)](
            auto &&stream_res) mutable {
          auto self = wp.lock();
          if (not self) {
            cb(Error::GONE);
            return;
          }

          if (not stream_res.has_value()) {
            cb(stream_res.as_failure());
            return;
          }
          auto &stream = stream_res.value();

          self->writeHandshake(
              std::move(stream), Direction::OUTGOING, std::move(cb));
        });
  }

  void PropagateTransactionsProtocol::readHandshake(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<NoData>(
        [stream, direction, wp = weak_from_this(), cb = std::move(cb)](
            auto &&remote_handshake_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(Error::GONE);
            return;
          }

          if (not remote_handshake_res.has_value()) {
            self->log_->error("Error while reading handshake: {}",
                              remote_handshake_res.error().message());
            stream->reset();
            cb(remote_handshake_res.as_failure());
            return;
          }

          switch (direction) {
            case Direction::OUTGOING:
              std::ignore = self->stream_engine_->addOutgoing(stream, self);
              self->readPropagatedExtrinsics(stream);
              cb(stream);
              break;
            case Direction::INCOMING:
              self->writeHandshake(std::move(stream), direction, std::move(cb));
              break;
          }
        });
  }

  void PropagateTransactionsProtocol::writeHandshake(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->write(
        NoData{},
        [stream, direction, wp = weak_from_this(), cb = std::move(cb)](
            auto &&write_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(Error::GONE);
            return;
          }

          if (not write_res.has_value()) {
            self->log_->error("Error while writing handshake: {}",
                              write_res.error().message());
            stream->reset();
            cb(write_res.as_failure());
            return;
          }

          switch (direction) {
            case Direction::OUTGOING:
              self->readHandshake(std::move(stream), direction, std::move(cb));
              break;
            case Direction::INCOMING:
              self->readPropagatedExtrinsics(std::move(stream));
              cb(stream);
              break;
          }
        });
  }

  void PropagateTransactionsProtocol::readPropagatedExtrinsics(
      std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<PropagatedExtrinsics>(
        [stream, wp = weak_from_this()](auto &&message_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }

          if (not message_res.has_value()) {
            self->log_->error("Error while reading propagated transactions: {}",
                              message_res.error().message());
            stream->reset();
            return;
          }
          auto &message = message_res.value();

          self->log_->info("Received {} propagated transactions",
                           message.extrinsics.size());
          for (auto &ext : message.extrinsics) {
            auto result = self->extrinsic_observer_->onTxMessage(ext);
            if (result) {
              self->log_->debug("  Received tx {}", result.value());
            } else {
              self->log_->debug("  Rejected tx: {}", result.error().message());
            }
          }

          self->readPropagatedExtrinsics(std::move(stream));
        });
  }

  void PropagateTransactionsProtocol::writePropagatedExtrinsics(
      std::shared_ptr<Stream> stream,
      const PropagatedExtrinsics &message,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->write(message,
                       [stream, wp = weak_from_this(), cb = std::move(cb)](
                           auto &&write_res) mutable {
                         auto self = wp.lock();
                         if (not self) {
                           stream->reset();
                           if (cb) cb(Error::GONE);
                           return;
                         }

                         if (not write_res.has_value()) {
                           self->log_->error(
                               "Error while writing propagated extrinsics: {}",
                               write_res.error().message());
                           stream->reset();
                           if (cb) cb(write_res.as_failure());
                           return;
                         }

                         if (cb) cb(stream);
                       });
  }

}  // namespace kagome::network
