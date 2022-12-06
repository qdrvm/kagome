/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/propagate_transactions_protocol.hpp"

#include "application/app_configuration.hpp"
#include "network/common.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/types/no_data_message.hpp"

namespace {
  constexpr const char *kPropagatedTransactions =
      "kagome_sync_propagated_transactions";
}

namespace kagome::network {

  KAGOME_DEFINE_CACHE(PropagateTransactionsProtocol);

  PropagateTransactionsProtocol::PropagateTransactionsProtocol(
      libp2p::Host &host,
      const application::AppConfiguration &app_config,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<consensus::babe::Babe> babe,
      std::shared_ptr<ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<StreamEngine> stream_engine,
      std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
          extrinsic_events_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          ext_event_key_repo)
      : base_(host,
              {fmt::format(kPropagateTransactionsProtocol.data(),
                           chain_spec.protocolId())},
              log::createLogger("PropagateTransactionsProtocol",
                                "propagate_transactions_protocol")),
        app_config_(app_config),
        babe_(std::move(babe)),
        extrinsic_observer_(std::move(extrinsic_observer)),
        stream_engine_(std::move(stream_engine)),
        extrinsic_events_engine_{std::move(extrinsic_events_engine)},
        ext_event_key_repo_{std::move(ext_event_key_repo)} {
    BOOST_ASSERT(extrinsic_observer_ != nullptr);
    BOOST_ASSERT(stream_engine_ != nullptr);
    BOOST_ASSERT(extrinsic_events_engine_ != nullptr);
    BOOST_ASSERT(ext_event_key_repo_ != nullptr);

    // Register metrics
    metrics_registry_->registerCounterFamily(
        kPropagatedTransactions,
        "Number of transactions propagated to at least one peer");
    metric_propagated_tx_counter_ =
        metrics_registry_->registerCounterMetric(kPropagatedTransactions);
  }

  bool PropagateTransactionsProtocol::start() {
    return base_.start(weak_from_this());
  }

  bool PropagateTransactionsProtocol::stop() {
    return base_.stop();
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
            SL_VERBOSE(self->base_.logger(),
                       "Handshake failed on incoming {} stream with {}: {}",
                       self->protocolName(),
                       peer_id,
                       res.error());
            stream->reset();
            return;
          }

          res = self->stream_engine_->addIncoming(stream, self);
          if (not res.has_value()) {
            SL_VERBOSE(self->base_.logger(),
                       "Can't register incoming {} stream with {}: {}",
                       self->protocolName(),
                       peer_id,
                       res.error());
            stream->reset();
            return;
          }

          SL_VERBOSE(self->base_.logger(),
                     "Fully established incoming {} stream with {}",
                     self->protocolName(),
                     peer_id);
        });
  }

  void PropagateTransactionsProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    base_.host().newStream(
        peer_info.id,
        base_.protocolIds(),
        [wp = weak_from_this(), peer_id = peer_info.id, cb = std::move(cb)](
            auto &&stream_res) mutable {
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
                       stream_res.error());
            cb(stream_res.as_failure());
            return;
          }
          const auto &stream_and_proto = stream_res.value();

          auto cb2 = [wp,
                      stream = stream_and_proto.stream,
                      protocol = stream_and_proto.protocol,
                      cb = std::move(cb)](outcome::result<void> res) {
            auto self = wp.lock();
            if (not self) {
              cb(ProtocolError::GONE);
              return;
            }

            if (not res.has_value()) {
              SL_VERBOSE(self->base_.logger(),
                         "Handshake failed on outgoing {} stream with {}: {}",
                         protocol,
                         stream->remotePeerId().value(),
                         res.error());
              stream->reset();
              cb(res.as_failure());
              return;
            }

            res = self->stream_engine_->addOutgoing(stream, self);
            if (not res.has_value()) {
              SL_VERBOSE(self->base_.logger(),
                         "Can't register outgoing {} stream with {}: {}",
                         protocol,
                         stream->remotePeerId().value(),
                         res.error());
              stream->reset();
              cb(res.as_failure());
              return;
            }

            SL_VERBOSE(self->base_.logger(),
                       "Fully established outgoing {} stream with {}",
                       protocol,
                       stream->remotePeerId().value());
            cb(std::move(stream));
          };

          self->writeHandshake(std::move(stream_and_proto.stream),
                               Direction::OUTGOING,
                               std::move(cb2));
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
            SL_VERBOSE(self->base_.logger(),
                       "Can't read handshake from {}: {}",
                       stream->remotePeerId().value(),
                       remote_handshake_res.error());
            stream->reset();
            cb(remote_handshake_res.as_failure());
            return;
          }

          SL_TRACE(self->base_.logger(),
                   "Handshake has received from {}",
                   stream->remotePeerId().value());

          switch (direction) {
            case Direction::OUTGOING:
              cb(outcome::success());
              break;
            case Direction::INCOMING:
              self->writeHandshake(
                  std::move(stream), Direction::INCOMING, std::move(cb));
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
                           SL_VERBOSE(self->base_.logger(),
                                      "Can't send handshake to {}: {}",
                                      stream->remotePeerId().value(),
                                      write_res.error());
                           stream->reset();
                           cb(write_res.as_failure());
                           return;
                         }

                         SL_TRACE(self->base_.logger(),
                                  "Handshake has sent to {}",
                                  stream->remotePeerId().value());

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
        SL_VERBOSE(self->base_.logger(),
                   "Can't read propagated transactions from {}: {}",
                   stream->remotePeerId().value(),
                   message_res.error());
        stream->reset();
        return;
      }

      auto peer_id = stream->remotePeerId().value();
      auto &message = message_res.value();

      SL_VERBOSE(self->base_.logger(),
                 "Received {} propagated transactions from {}",
                 message.extrinsics.size(),
                 peer_id);

      if (self->babe_->wasSynchronized()) {
        for (auto &ext : message.extrinsics) {
          auto result = self->extrinsic_observer_->onTxMessage(ext);
          if (result) {
            SL_DEBUG(self->base_.logger(), "  Received tx {}", result.value());
          } else {
            SL_DEBUG(self->base_.logger(), "  Rejected tx: {}", result.error());
          }
        }
      } else {
        SL_TRACE(self->base_.logger(),
                 "Skipping extrinsics processing since the node was not in a "
                 "synchronized state yet.");
      }

      self->readPropagatedExtrinsics(std::move(stream));
    });
  }

  void PropagateTransactionsProtocol::propagateTransactions(
      gsl::span<const primitives::Transaction> txs) {
    SL_DEBUG(
        base_.logger(), "Propagate transactions : {} extrinsics", txs.size());

    std::vector<libp2p::peer::PeerId> peers;
    stream_engine_->forEachPeer(
        [&peers](const libp2p::peer::PeerId &peer_id, const auto &) {
          peers.push_back(peer_id);
        });
    if (peers.size() > 1) {  // One of peers is current node itself
      metric_propagated_tx_counter_->inc(peers.size() - 1);
      for (const auto &tx : txs) {
        if (auto key = ext_event_key_repo_->get(tx.hash); key.has_value()) {
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

    stream_engine_->broadcast<PropagatedExtrinsics>(
        shared_from_this(),
        shared_msg,
        StreamEngine::RandomGossipStrategy{
            stream_engine_->outgoingStreamsNumber(shared_from_this()),
            app_config_.luckyPeers()});
  }

}  // namespace kagome::network
