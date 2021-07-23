/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/protocols/propagate_transactions_protocol.hpp"

#include "network/common.hpp"
#include "network/protocols/protocol_error.hpp"
#include "network/types/no_data_message.hpp"

namespace kagome::network {

  KAGOME_DEFINE_CACHE(PropagateTransactionsProtocol);

  PropagateTransactionsProtocol::PropagateTransactionsProtocol(
      libp2p::Host &host,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<consensus::babe::Babe> babe,
      std::shared_ptr<ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<StreamEngine> stream_engine,
      std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
          extrinsic_events_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          ext_event_key_repo)
      : host_(host),
        babe_(std::move(babe)),
        extrinsic_observer_(std::move(extrinsic_observer)),
        stream_engine_(std::move(stream_engine)),
        extrinsic_events_engine_{std::move(extrinsic_events_engine)},
        ext_event_key_repo_{std::move(ext_event_key_repo)} {
    BOOST_ASSERT(extrinsic_observer_ != nullptr);
    BOOST_ASSERT(stream_engine_ != nullptr);
    BOOST_ASSERT(extrinsic_events_engine_ != nullptr);
    BOOST_ASSERT(ext_event_key_repo_ != nullptr);
    const_cast<Protocol &>(protocol_) = fmt::format(
        kPropagateTransactionsProtocol.data(), chain_spec.protocolId());
  }

  bool PropagateTransactionsProtocol::start() {
    host_.setProtocolHandler(protocol_, [wp = weak_from_this()](auto &&stream) {
      if (auto self = wp.lock()) {
        if (auto peer_id = stream->remotePeerId()) {
          SL_TRACE(self->log_,
                   "Handled {} protocol stream from: {}",
                   self->protocol_,
                   peer_id.value().toBase58());
          self->onIncomingStream(std::forward<decltype(stream)>(stream));
          return;
        }
        self->log_->warn("Handled {} protocol stream from unknown peer",
                         self->protocol_);
        stream->reset();
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
        [wp = weak_from_this(), stream](outcome::result<void> res) {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }

          auto peer_id = stream->remotePeerId().value();

          if (not res.has_value()) {
            SL_VERBOSE(self->log_,
                       "Handshake failed on incoming {} stream with {}: {}",
                       self->protocol_,
                       peer_id.toBase58(),
                       res.error().message());
            stream->reset();
            return;
          }

          res = self->stream_engine_->addIncoming(stream, self);
          if (not res.has_value()) {
            SL_VERBOSE(self->log_,
                       "Can't register incoming {} stream with {}: {}",
                       self->protocol_,
                       peer_id.toBase58(),
                       res.error().message());
            stream->reset();
            return;
          }

          SL_VERBOSE(self->log_,
                     "Fully established incoming {} stream with {}",
                     self->protocol_,
                     peer_id.toBase58());
        });
  }

  void PropagateTransactionsProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    host_.newStream(
        peer_info.id,
        protocol_,
        [wp = weak_from_this(), peer_id = peer_info.id, cb = std::move(cb)](
            auto &&stream_res) mutable {
          auto self = wp.lock();
          if (not self) {
            cb(ProtocolError::GONE);
            return;
          }

          if (not stream_res.has_value()) {
            SL_VERBOSE(self->log_,
                       "Can't create outgoing {} stream with {}: {}",
                       self->protocol_,
                       peer_id.toBase58(),
                       stream_res.error().message());
            cb(stream_res.as_failure());
            return;
          }
          auto &stream = stream_res.value();

          auto cb2 = [wp, stream, cb = std::move(cb)](
                         outcome::result<void> res) {
            auto self = wp.lock();
            if (not self) {
              cb(ProtocolError::GONE);
              return;
            }

            if (not res.has_value()) {
              SL_VERBOSE(self->log_,
                         "Handshake failed on outgoing {} stream with {}: {}",
                         self->protocol_,
                         stream->remotePeerId().value().toBase58(),
                         res.error().message());
              stream->reset();
              cb(res.as_failure());
              return;
            }

            res = self->stream_engine_->addOutgoing(stream, self);
            if (not res.has_value()) {
              SL_VERBOSE(self->log_,
                         "Can't register outgoing {} stream with {}: {}",
                         self->protocol_,
                         stream->remotePeerId().value().toBase58(),
                         res.error().message());
              stream->reset();
              cb(res.as_failure());
              return;
            }

            SL_VERBOSE(self->log_,
                       "Fully established outgoing {} stream with {}",
                       self->protocol_,
                       stream->remotePeerId().value().toBase58());
            cb(std::move(stream));
          };

          self->writeHandshake(
              std::move(stream), Direction::OUTGOING, std::move(cb2));
        });
  }

  void PropagateTransactionsProtocol::readHandshake(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<NoData>(
        [stream, direction, wp = weak_from_this(), cb = std::move(cb)](
            auto &&remote_handshake_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(ProtocolError::GONE);
            return;
          }

          if (not remote_handshake_res.has_value()) {
            SL_VERBOSE(self->log_,
                       "Can't read handshake from {}: {}",
                       stream->remotePeerId().value().toBase58(),
                       remote_handshake_res.error().message());
            stream->reset();
            cb(remote_handshake_res.as_failure());
            return;
          }

          SL_TRACE(self->log_,
                   "Handshake has received from {}",
                   stream->remotePeerId().value().toBase58());

          switch (direction) {
            case Direction::OUTGOING:
              cb(outcome::success());
              break;
            case Direction::INCOMING:
              if (self->babe_->getCurrentState()
                  == consensus::babe::Babe::State::SYNCHRONIZED) {
                self->writeHandshake(
                    std::move(stream), Direction::INCOMING, std::move(cb));
                return;
              }

              stream->close([](auto &&...) {});
              cb(ProtocolError::NODE_NOT_SYNCHRONIZED_YET);
              break;
          }
        });
  }

  void PropagateTransactionsProtocol::writeHandshake(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->write(NoData{},
                       [stream = std::move(stream),
                        direction,
                        wp = weak_from_this(),
                        cb = std::move(cb)](auto &&write_res) mutable {
                         auto self = wp.lock();
                         if (not self) {
                           stream->reset();
                           cb(ProtocolError::GONE);
                           return;
                         }

                         if (not write_res.has_value()) {
                           SL_VERBOSE(self->log_,
                                      "Can't send handshake to {}: {}",
                                      stream->remotePeerId().value().toBase58(),
                                      write_res.error().message());
                           stream->reset();
                           cb(write_res.as_failure());
                           return;
                         }

                         SL_TRACE(self->log_,
                                  "Handshake has sent to {}",
                                  stream->remotePeerId().value().toBase58());

                         switch (direction) {
                           case Direction::OUTGOING:
                             self->readHandshake(std::move(stream),
                                                 Direction::OUTGOING,
                                                 std::move(cb));
                             break;
                           case Direction::INCOMING:
                             cb(outcome::success());
                             self->readPropagatedExtrinsics(std::move(stream));
                             break;
                         }
                       });
  }

  void PropagateTransactionsProtocol::readPropagatedExtrinsics(
      std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<PropagatedExtrinsics>([stream = std::move(stream),
                                             wp = weak_from_this()](
                                                auto &&message_res) mutable {
      auto self = wp.lock();
      if (not self) {
        stream->reset();
        return;
      }

      if (not message_res.has_value()) {
        SL_VERBOSE(self->log_,
                   "Can't read grandpa message from {}: {}",
                   stream->remotePeerId().value().toBase58(),
                   message_res.error().message());
        stream->reset();
        return;
      }

      auto peer_id = stream->remotePeerId().value();
      auto &message = message_res.value();

      SL_VERBOSE(self->log_,
                 "Received {} propagated transactions from {}",
                 message.extrinsics.size(),
                 peer_id.toBase58());

      for (auto &ext : message.extrinsics) {
        auto result = self->extrinsic_observer_->onTxMessage(ext);
        if (result) {
          SL_DEBUG(self->log_, "  Received tx {}", result.value());
        } else {
          SL_DEBUG(self->log_, "  Rejected tx: {}", result.error().message());
        }
      }

      self->readPropagatedExtrinsics(std::move(stream));
    });
  }

  void PropagateTransactionsProtocol::propagateTransactions(
      gsl::span<const primitives::Transaction> txs) {
    SL_DEBUG(log_, "Propagate transactions : {} extrinsics", txs.size());

    std::vector<libp2p::peer::PeerId> peers;
    stream_engine_->forEachPeer(
        [&peers](const libp2p::peer::PeerId &peer_id, const auto &) {
          peers.push_back(peer_id);
        });
    if (peers.size() > 1) {  // One of peers is current node itself
      for (const auto &tx : txs) {
        if (auto key = ext_event_key_repo_->getEventKey(tx); key.has_value()) {
          extrinsic_events_engine_->notify(
              key.value(),
              primitives::events::ExtrinsicLifecycleEvent::Broadcast(
                  key.value(), peers));
        }
      }
    }

    PropagatedExtrinsics exts;
    exts.extrinsics.resize(txs.size());
    std::transform(
        txs.begin(), txs.end(), exts.extrinsics.begin(), [](auto &tx) {
          return tx.ext;
        });

    auto shared_msg = KAGOME_EXTRACT_SHARED_CACHE(PropagateTransactionsProtocol,
                                                  PropagatedExtrinsics);
    (*shared_msg) = std::move(exts);

    stream_engine_->broadcast<PropagatedExtrinsics>(shared_from_this(),
                                                    std::move(shared_msg));
  }

}  // namespace kagome::network
