/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/propagate_transactions_protocol.hpp"

#include <algorithm>

#include "blockchain/genesis_block_hash.hpp"
#include "consensus/timeline/timeline.hpp"
#include "network/common.hpp"
#include "network/notifications/connect_and_handshake.hpp"
#include "network/notifications/handshake_and_read_messages.hpp"

namespace {
  constexpr const char *kPropagatedTransactions =
      "kagome_sync_propagated_transactions";
}

namespace kagome::network {

  KAGOME_DEFINE_CACHE(PropagateTransactionsProtocol);

  PropagateTransactionsProtocol::PropagateTransactionsProtocol(
      libp2p::Host &host,
      Roles roles,
      const application::ChainSpec &chain_spec,
      const blockchain::GenesisBlockHash &genesis_hash,
      std::shared_ptr<consensus::Timeline> timeline,
      std::shared_ptr<ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<StreamEngine> stream_engine,
      std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
          extrinsic_events_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          ext_event_key_repo)
      : base_(kPropagateTransactionsProtocolName,
              host,
              make_protocols(
                  kPropagateTransactionsProtocol, genesis_hash, chain_spec),
              log::createLogger(kPropagateTransactionsProtocolName,
                                "propagate_transactions_protocol")),
        roles_{roles},
        timeline_(std::move(timeline)),
        extrinsic_observer_(std::move(extrinsic_observer)),
        stream_engine_(std::move(stream_engine)),
        extrinsic_events_engine_{std::move(extrinsic_events_engine)},
        ext_event_key_repo_{std::move(ext_event_key_repo)} {
    BOOST_ASSERT(timeline_ != nullptr);
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

  const ProtocolName &PropagateTransactionsProtocol::protocolName() const {
    return base_.protocolName();
  }

  void PropagateTransactionsProtocol::onIncomingStream(
      std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());
    auto on_handshake = [](std::shared_ptr<PropagateTransactionsProtocol> self,
                           std::shared_ptr<Stream> stream,
                           Roles) {
      self->stream_engine_->addIncoming(stream, self);
      return true;
    };
    auto on_message = [peer_id = stream->remotePeerId().value()](
                          std::shared_ptr<PropagateTransactionsProtocol> self,
                          PropagatedExtrinsics message) {
      SL_VERBOSE(self->base_.logger(),
                 "Received {} propagated transactions from {}",
                 message.extrinsics.size(),
                 peer_id);

      if (self->timeline_->wasSynchronized()) {
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
      return true;
    };
    notifications::handshakeAndReadMessages<PropagatedExtrinsics>(
        weak_from_this(),
        std::move(stream),
        roles_,
        std::move(on_handshake),
        std::move(on_message));
  }

  void PropagateTransactionsProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto on_handshake =
        [cb = std::move(cb)](
            std::shared_ptr<PropagateTransactionsProtocol> self,
            outcome::result<notifications::ConnectAndHandshake<Roles>>
                r) mutable {
          if (not r) {
            cb(r.error());
            return;
          }
          auto &stream = std::get<0>(r.value());
          self->stream_engine_->addOutgoing(stream, self);
          cb(std::move(stream));
        };
    notifications::connectAndHandshake(
        weak_from_this(), base_, peer_info, roles_, std::move(on_handshake));
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

    auto propagated_exts = KAGOME_EXTRACT_SHARED_CACHE(
        PropagateTransactionsProtocol, PropagatedExtrinsics);
    propagated_exts->extrinsics.resize(txs.size());
    std::transform(txs.begin(),
                   txs.end(),
                   propagated_exts->extrinsics.begin(),
                   [](auto &tx) { return tx.ext; });
    stream_engine_->broadcast<PropagatedExtrinsics>(shared_from_this(),
                                                    propagated_exts);
  }

}  // namespace kagome::network
