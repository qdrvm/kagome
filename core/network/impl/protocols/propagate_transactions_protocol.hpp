/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "network/extrinsic_observer.hpp"
#include "network/notifications/protocol.hpp"
#include "network/types/propagate_transactions.hpp"
#include "network/types/roles.hpp"
#include "primitives/event_types.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"
#include "subscription/subscriber.hpp"
#include "subscription/subscription_engine.hpp"
#include "utils/lru.hpp"
#include "utils/non_copyable.hpp"

namespace kagome {
  class PoolHandler;
}  // namespace kagome

namespace kagome::application {
  class ChainSpec;
}  // namespace kagome::application

namespace kagome::blockchain {
  class GenesisBlockHash;
}  // namespace kagome::blockchain

namespace kagome::common {
  class MainThreadPool;
}  // namespace kagome::common

namespace kagome::consensus {
  class Timeline;
}  // namespace kagome::consensus

namespace kagome::crypto {
  class Hasher;
}  // namespace kagome::crypto

namespace kagome::network {
  using libp2p::PeerId;

  class PropagateTransactionsProtocol final
      : public std::enable_shared_from_this<PropagateTransactionsProtocol>,
        public notifications::Controller,
        NonCopyable,
        NonMovable {
   public:
    PropagateTransactionsProtocol(
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
            ext_event_key_repo);

    // Controller
    Buffer handshake() override;
    bool onHandshake(const PeerId &peer_id,
                     size_t,
                     bool,
                     Buffer &&handshake) override;
    bool onMessage(const PeerId &peer_id, size_t, Buffer &&message) override;
    void onClose(const PeerId &peer_id) override;

    void start();

    void propagateTransaction(primitives::Transaction tx);

   private:
    static constexpr auto kPropagateTransactionsProtocolName =
        "PropagateTransactionsProtocol";

    log::Logger log_;
    std::shared_ptr<notifications::Protocol> notifications_;
    Roles roles_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<PoolHandler> main_pool_handler_;
    std::shared_ptr<consensus::Timeline> timeline_;
    std::shared_ptr<ExtrinsicObserver> extrinsic_observer_;
    std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
        extrinsic_events_engine_;
    std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
        ext_event_key_repo_;
    MapLruSet<PeerId, Hash256> seen_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Counter *metric_propagated_tx_counter_;

    friend class BlockAnnounceProtocol;
  };

}  // namespace kagome::network
