/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/production_consensus.hpp"

#include "clock/clock.hpp"
#include "consensus/babe/types/babe_configuration.hpp"
#include "consensus/babe/types/slot_leadership.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "primitives/block.hpp"
#include "primitives/event_types.hpp"
#include "telemetry/service.hpp"

namespace kagome {
  class PoolHandler;
}  // namespace kagome

namespace kagome::application {
  class AppConfiguration;
  class AppStateManager;
}  // namespace kagome::application

namespace kagome::authorship {
  class Proposer;
}

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::common {
  class WorkerThreadPool;
  class MainThreadPool;
}  // namespace kagome::common

namespace kagome::consensus {
  class SlotsUtil;
}

namespace kagome::consensus::babe {
  class BabeConfigRepository;
  class BabeBlockValidator;
  class BabeLottery;
}  // namespace kagome::consensus::babe

namespace kagome::crypto {
  class SessionKeys;
  class Hasher;
  class Sr25519Provider;
}  // namespace kagome::crypto

namespace kagome::dispute {
  class DisputeCoordinator;
}

namespace kagome::network {
  class BlockAnnounceTransmitter;
}

namespace kagome::offchain {
  class OffchainWorkerFactory;
  class OffchainWorkerPool;
}  // namespace kagome::offchain

namespace kagome::parachain {
  class BitfieldStore;
  class ParachainProcessorImpl;
  struct BackedCandidatesSource;
}  // namespace kagome::parachain

namespace kagome::runtime {
  class BabeApi;
  class OffchainWorkerApi;
}  // namespace kagome::runtime

namespace kagome::storage::changes_trie {
  class StorageChangesTrackerImpl;
}

namespace kagome::consensus::babe {

  /// BABE protocol, used for block production in the Polkadot consensus.
  class Babe : public ProductionConsensus,
               public std::enable_shared_from_this<Babe> {
   public:
    struct Context {
      primitives::BlockInfo parent;
      EpochNumber epoch;
      SlotNumber slot;
      TimePoint slot_timestamp;
      std::shared_ptr<crypto::Sr25519Keypair> keypair;
    };

    Babe(
        application::AppStateManager &app_state_manager,
        const application::AppConfiguration &app_config,
        const clock::SystemClock &clock,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        LazySPtr<SlotsUtil> slots_util,
        std::shared_ptr<BabeConfigRepository> config_repo,
        const EpochTimings &timings,
        std::shared_ptr<crypto::SessionKeys> session_keys,
        std::shared_ptr<BabeLottery> lottery,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
        std::shared_ptr<BabeBlockValidator> validating,
        std::shared_ptr<parachain::BitfieldStore> bitfield_store,
        std::shared_ptr<parachain::BackedCandidatesSource> candidates_source,
        std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator,
        std::shared_ptr<authorship::Proposer> proposer,
        primitives::events::StorageSubscriptionEnginePtr storage_sub_engine,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        std::shared_ptr<network::BlockAnnounceTransmitter> announce_transmitter,
        std::shared_ptr<runtime::BabeApi> babe_api,
        std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
        std::shared_ptr<offchain::OffchainWorkerFactory>
            offchain_worker_factory,
        std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool,
        common::MainThreadPool &main_thread_pool,
        common::WorkerThreadPool &worker_thread_pool);

    bool isGenesisConsensus() const override;

    ValidatorStatus getValidatorStatus(const primitives::BlockInfo &parent_info,
                                       EpochNumber epoch_number) const override;

    outcome::result<SlotNumber> getSlot(
        const primitives::BlockHeader &header) const override;

    outcome::result<AuthorityIndex> getAuthority(
        const primitives::BlockHeader &header) const override;

    outcome::result<void> processSlot(
        SlotNumber slot, const primitives::BlockInfo &best_block) override;

    outcome::result<void> validateHeader(
        const primitives::BlockHeader &block_header) const override;

    outcome::result<void> reportEquivocation(
        const primitives::BlockHash &first,
        const primitives::BlockHash &second) const override;

   private:
    bool changeEpoch(EpochNumber epoch,
                     const primitives::BlockInfo &block) const override;

    bool checkSlotLeadership(const primitives::BlockInfo &block,
                             SlotNumber slot) override;

    outcome::result<void> processSlotLeadership();

    outcome::result<primitives::PreRuntime> makePreDigest() const override;

    outcome::result<primitives::Seal> makeSeal(
        const primitives::Block &block) const override;

   protected:
    virtual outcome::result<void> processSlotLeadershipProposed(
        uint64_t now,
        clock::SteadyClock::TimePoint proposal_start,
        std::shared_ptr<storage::changes_trie::StorageChangesTrackerImpl>
            &&changes_tracker,
        primitives::Block &&block);

   private:
    log::Logger log_;

    const clock::SystemClock &clock_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    LazySPtr<SlotsUtil> slots_util_;
    std::shared_ptr<BabeConfigRepository> config_repo_;
    const EpochTimings &timings_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<BabeLottery> lottery_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
    std::shared_ptr<BabeBlockValidator> validating_;
    std::shared_ptr<parachain::BitfieldStore> bitfield_store_;
    std::shared_ptr<parachain::BackedCandidatesSource> candidates_source_;
    std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator_;
    std::shared_ptr<authorship::Proposer> proposer_;
    primitives::events::StorageSubscriptionEnginePtr storage_sub_engine_;
    primitives::events::ChainSubscriptionEnginePtr chain_sub_engine_;
    std::shared_ptr<network::BlockAnnounceTransmitter> announce_transmitter_;
    std::shared_ptr<runtime::BabeApi> babe_api_;
    std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api_;
    std::shared_ptr<offchain::OffchainWorkerFactory> offchain_worker_factory_;
    std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool_;
    std::shared_ptr<PoolHandler> main_pool_handler_;
    std::shared_ptr<PoolHandler> worker_pool_handler_;

    const bool is_validator_by_config_;
    bool is_active_validator_{};

    using KeypairWithIndexOpt = std::optional<
        std::pair<std::shared_ptr<crypto::Sr25519Keypair>, AuthorityIndex>>;

    primitives::BlockInfo parent_;
    TimePoint slot_timestamp_;
    SlotNumber slot_{};
    EpochNumber epoch_{};
    SlotLeadership slot_leadership_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_is_relaychain_validator_;

    telemetry::Telemetry telemetry_;  // telemetry
  };

}  // namespace kagome::consensus::babe
