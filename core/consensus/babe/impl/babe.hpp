/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/production_consensus.hpp"

#include "clock/clock.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "primitives/block.hpp"
#include "primitives/event_types.hpp"
#include "primitives/inherent_data.hpp"
#include "telemetry/service.hpp"

namespace boost::asio {
  class io_context;
}

namespace kagome {
  class ThreadPool;
}

namespace kagome::application {
  class AppConfiguration;
}

namespace kagome::authorship {
  class Proposer;
}

namespace kagome::blockchain {
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::consensus {
  class SlotsUtil;
}

namespace kagome::consensus::babe {
  class BabeConfigRepository;
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

namespace kagome::parachain {
  class BitfieldStore;
  class BackingStore;
}  // namespace kagome::parachain

namespace kagome::network {
  class BlockAnnounceTransmitter;
}

namespace kagome::runtime {
  class OffchainWorkerApi;
}

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
        const application::AppConfiguration &app_config,
        const clock::SystemClock &clock,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        LazySPtr<SlotsUtil> slots_util,
        std::shared_ptr<BabeConfigRepository> babe_config_repo,
        std::shared_ptr<crypto::SessionKeys> session_keys,
        std::shared_ptr<BabeLottery> lottery,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
        std::shared_ptr<parachain::BitfieldStore> bitfield_store,
        std::shared_ptr<parachain::BackingStore> backing_store,
        std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator,
        std::shared_ptr<authorship::Proposer> proposer,
        primitives::events::StorageSubscriptionEnginePtr storage_sub_engine,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        std::shared_ptr<network::BlockAnnounceTransmitter> announce_transmitter,
        std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
        const ThreadPool &thread_pool,
        std::shared_ptr<boost::asio::io_context> main_thread);

    ValidatorStatus getValidatorStatus(const primitives::BlockInfo &parent_info,
                                       EpochNumber epoch_number) const override;

    std::tuple<Duration, EpochLength> getTimings() const override;

    outcome::result<SlotNumber> getSlot(
        const primitives::BlockHeader &header) const override;

    outcome::result<void> processSlot(
        SlotNumber slot, const primitives::BlockInfo &best_block) override;

   private:
    outcome::result<primitives::PreRuntime> babePreDigest(
        const Context &ctx,
        SlotType slot_type,
        std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
        primitives::AuthorityIndex authority_index) const;

    outcome::result<primitives::Seal> sealBlock(
        const Context &ctx, const primitives::Block &block) const;

    outcome::result<void> processSlotLeadership(
        const Context &ctx,
        SlotType slot_type,
        TimePoint slot_timestamp,
        std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
        primitives::AuthorityIndex authority_index);

    /**
     * `processSlotLeadership` coroutine piece
     *   processSlotLeadership() {
     *     await propose()
     *     // processSlotLeadershipProposed()
     *   }
     */
    outcome::result<void> processSlotLeadershipProposed(
        const Context &ctx,
        uint64_t now,
        clock::SteadyClock::TimePoint proposal_start,
        std::shared_ptr<storage::changes_trie::StorageChangesTrackerImpl>
            &&changes_tracker,
        primitives::Block &&block);

    void changeLotteryEpoch(
        const Context &ctx,
        const EpochNumber &epoch,
        primitives::AuthorityIndex authority_index,
        const primitives::BabeConfiguration &babe_config) const;

    log::Logger log_;
    const clock::SystemClock &clock_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    LazySPtr<SlotsUtil> slots_util_;
    std::shared_ptr<BabeConfigRepository> babe_config_repo_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<BabeLottery> lottery_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
    std::shared_ptr<parachain::BitfieldStore> bitfield_store_;
    std::shared_ptr<parachain::BackingStore> backing_store_;
    std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator_;
    std::shared_ptr<authorship::Proposer> proposer_;
    primitives::events::StorageSubscriptionEnginePtr storage_sub_engine_;
    primitives::events::ChainSubscriptionEnginePtr chain_sub_engine_;
    std::shared_ptr<network::BlockAnnounceTransmitter> announce_transmitter_;
    std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api_;
    std::shared_ptr<boost::asio::io_context> main_thread_;
    std::shared_ptr<boost::asio::io_context> io_context_;

    const bool is_validator_by_config_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_is_relaychain_validator_;

    telemetry::Telemetry telemetry_;  // telemetry
  };

}  // namespace kagome::consensus::babe
