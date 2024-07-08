/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/timeline.hpp"

#include "application/sync_method.hpp"
#include "clock/clock.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "network/block_announce_observer.hpp"
#include "primitives/block_header.hpp"
#include "primitives/event_types.hpp"
#include "telemetry/service.hpp"

namespace kagome {
  class PoolHandler;
}  // namespace kagome

namespace kagome::application {
  class AppStateManager;
  class AppConfiguration;
}  // namespace kagome::application

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::common {
  class MainThreadPool;
}  // namespace kagome::common

namespace kagome::consensus {
  class SlotsUtil;
  class ConsensusSelector;
}  // namespace kagome::consensus

namespace kagome::consensus::grandpa {
  class JustificationObserver;
}

namespace kagome::network {
  class Synchronizer;
  class BlockAnnounceTransmitter;
  class WarpSync;
  class WarpProtocol;
}  // namespace kagome::network

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::storage::trie_pruner {
  class TriePruner;
}  // namespace kagome::storage::trie_pruner

namespace kagome::runtime {
  class Core;
}

namespace libp2p::basic {
  class Scheduler;
}

namespace kagome::consensus {

  class TimelineImpl final : public Timeline,
                             public network::BlockAnnounceObserver,
                             public std::enable_shared_from_this<TimelineImpl> {
    static const size_t kMaxSlotObserveForEquivocation = 1000;

   public:
    TimelineImpl(
        const application::AppConfiguration &app_config,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        const clock::SystemClock &clock,
        common::MainThreadPool &main_thread_pool,
        std::shared_ptr<SlotsUtil> slots_util,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<ConsensusSelector> consensus_selector,
        std::shared_ptr<storage::trie::TrieStorage> trie_storage,
        std::shared_ptr<storage::trie_pruner::TriePruner> trie_pruner,
        std::shared_ptr<network::Synchronizer> synchronizer,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<network::BlockAnnounceTransmitter>
            block_announce_transmitter,
        std::shared_ptr<network::WarpSync> warp_sync,
        LazySPtr<network::WarpProtocol> warp_protocol,
        std::shared_ptr<consensus::grandpa::JustificationObserver>
            justification_observer,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        primitives::events::SyncStateSubscriptionEnginePtr state_sub_engine,
        std::shared_ptr<runtime::Core> core_api);

    /// @see AppStateManager::takeControl
    bool prepare();

    /// @see AppStateManager::takeControl
    bool start();

    // Timeline's methods

    SyncState getCurrentState() const override;

    bool wasSynchronized() const override;

    // BlockAnnounceObserver's methods

    void onBlockAnnounceHandshake(
        const libp2p::peer::PeerId &peer_id,
        const network::BlockAnnounceHandshake &handshake) override;

    void onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                         const network::BlockAnnounce &announce) override;

    void checkAndReportEquivocation(
        const primitives::BlockHeader &header) override;

   private:
    bool updateSlot(TimePoint now);

    bool warpSync(const libp2p::peer::PeerId &peer_id,
                  primitives::BlockNumber block_number);

    void startCatchUp(const libp2p::peer::PeerId &peer_id,
                      const primitives::BlockInfo &target_block);

    void onCaughtUp(const primitives::BlockInfo &block);

    void startStateSyncing(const libp2p::peer::PeerId &peer_id);

    void onSynchronized();

    void runEpoch();

    void runSlot();

    void processSlot(TimePoint timestamp);

    log::Logger log_;

    std::shared_ptr<application::AppStateManager> app_state_manager_;
    const clock::SystemClock &clock_;
    std::shared_ptr<PoolHandler> main_pool_handler_;
    std::shared_ptr<SlotsUtil> slots_util_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<ConsensusSelector> consensus_selector_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;
    std::shared_ptr<storage::trie_pruner::TriePruner> trie_pruner_;
    std::shared_ptr<network::Synchronizer> synchronizer_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<network::BlockAnnounceTransmitter>
        block_announce_transmitter_;
    std::shared_ptr<network::WarpSync> warp_sync_;
    LazySPtr<network::WarpProtocol> warp_protocol_;
    std::shared_ptr<consensus::grandpa::JustificationObserver>
        justification_observer_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    primitives::events::ChainSubscriptionEnginePtr chain_sub_engine_;
    primitives::events::ChainSub chain_sub_;
    primitives::events::SyncStateSubscriptionEnginePtr state_sub_engine_;
    std::shared_ptr<runtime::Core> core_api_;

    application::SyncMethod sync_method_;

    SyncState current_state_{SyncState::WAIT_REMOTE_STATUS};
    bool was_synchronized_{false};

    EpochNumber current_epoch_;
    SlotNumber current_slot_{};
    primitives::BlockInfo best_block_{};

    bool warp_sync_busy_{false};

    std::atomic_bool active_{false};

    std::optional<primitives::Version> actual_runtime_version_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_is_major_syncing_;

    using AuthorityIndex = uint32_t;

    std::map<std::tuple<SlotNumber, AuthorityIndex>,
             std::tuple<primitives::BlockHash, bool>>
        data_for_equvocation_checks_;

    telemetry::Telemetry telemetry_;
  };

}  // namespace kagome::consensus
