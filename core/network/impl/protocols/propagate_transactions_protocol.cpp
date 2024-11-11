/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/propagate_transactions_protocol.hpp"

#include <algorithm>

#include "blockchain/genesis_block_hash.hpp"
#include "common/main_thread_pool.hpp"
#include "consensus/timeline/timeline.hpp"
#include "network/common.hpp"
#include "network/notifications/encode.hpp"
#include "utils/pool_handler.hpp"
#include "utils/try.hpp"

namespace {
  constexpr const char *kPropagatedTransactions =
      "kagome_sync_propagated_transactions";
}

namespace kagome::network {
  // https://github.com/paritytech/polkadot-sdk/blob/edf79aa972bcf2e043e18065a9bb860ecdbd1a6e/substrate/client/network/transactions/src/config.rs#L33
  constexpr size_t kSeenCapacity = 10240;

  PropagateTransactionsProtocol::PropagateTransactionsProtocol(
      const notifications::Factory &notifications_factory,
      Roles roles,
      std::shared_ptr<crypto::Hasher> hasher,
      const application::ChainSpec &chain_spec,
      const blockchain::GenesisBlockHash &genesis_hash,
      common::MainThreadPool &main_thread_pool,
      std::shared_ptr<consensus::Timeline> timeline,
      std::shared_ptr<ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
          extrinsic_events_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          ext_event_key_repo)
      : log_{log::createLogger(kPropagateTransactionsProtocolName,
                               "propagate_transactions_protocol")},
        notifications_{notifications_factory.make(
            {make_protocols(
                kPropagateTransactionsProtocol, genesis_hash, chain_spec)},
            0,
            0)},
        roles_{roles},
        main_pool_handler_{main_thread_pool.handlerStarted()},
        timeline_(std::move(timeline)),
        extrinsic_observer_(std::move(extrinsic_observer)),
        extrinsic_events_engine_{std::move(extrinsic_events_engine)},
        ext_event_key_repo_{std::move(ext_event_key_repo)},
        seen_{kSeenCapacity} {
    BOOST_ASSERT(main_pool_handler_ != nullptr);
    BOOST_ASSERT(timeline_ != nullptr);
    BOOST_ASSERT(extrinsic_observer_ != nullptr);
    BOOST_ASSERT(extrinsic_events_engine_ != nullptr);
    BOOST_ASSERT(ext_event_key_repo_ != nullptr);

    // Register metrics
    metrics_registry_->registerCounterFamily(
        kPropagatedTransactions,
        "Number of transactions propagated to at least one peer");
    metric_propagated_tx_counter_ =
        metrics_registry_->registerCounterMetric(kPropagatedTransactions);
  }

  Buffer PropagateTransactionsProtocol::handshake() {
    return scale::encode(roles_).value();
  }

  bool PropagateTransactionsProtocol::onHandshake(const PeerId &peer_id,
                                                  size_t,
                                                  bool,
                                                  Buffer &&handshake) {
    TRY_FALSE(scale::decode<Roles>(handshake));
    return true;
  }

  bool PropagateTransactionsProtocol::onMessage(const PeerId &peer_id,
                                                size_t,
                                                Buffer &&message_raw) {
    auto message = TRY_FALSE(scale::decode<PropagatedExtrinsics>(message_raw));
    SL_VERBOSE(log_,
               "Received {} propagated transactions from {}",
               message.extrinsics.size(),
               peer_id);

    if (timeline_->wasSynchronized()) {
      for (auto &ext : message.extrinsics) {
        auto hash = hasher_->blake2b_256(ext.data);
        if (not seen_.add(peer_id, hash)) {
          continue;
        }
        auto result = extrinsic_observer_->onTxMessage(ext);
        if (result) {
          SL_DEBUG(log_, "  Received tx {}", result.value());
        } else {
          SL_DEBUG(log_, "  Rejected tx: {}", result.error());
        }
      }
    } else {
      SL_TRACE(log_,
               "Skipping extrinsics processing since the node was not in a "
               "synchronized state yet.");
    }
    return true;
  }

  void PropagateTransactionsProtocol::onClose(const PeerId &peer_id) {
    seen_.remove(peer_id);
  }

  void PropagateTransactionsProtocol::start() {
    notifications_->start(weak_from_this());
  }

  void PropagateTransactionsProtocol::propagateTransaction(
      primitives::Transaction tx) {
    REINVOKE(*main_pool_handler_, propagateTransaction, std::move(tx));
    SL_DEBUG(log_, "Propagate transaction");
    std::vector<PeerId> peers;
    size_t metric = 0;
    auto message_raw = notifications::encode(PropagatedExtrinsics{{tx.ext}});
    notifications_->peersOut([&](const PeerId &peer_id, size_t) {
      if (not seen_.add(peer_id, tx.hash)) {
        return true;
      }
      notifications_->write(peer_id, message_raw);
      ++metric;
      peers.emplace_back(peer_id);
      return true;
    });
    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
    metric_propagated_tx_counter_->inc(metric);
    if (auto key = ext_event_key_repo_->get(tx.hash); key.has_value()) {
      extrinsic_events_engine_->notify(
          key.value(),
          primitives::events::ExtrinsicLifecycleEvent::Broadcast(key.value(),
                                                                 peers));
    }
  }
}  // namespace kagome::network
