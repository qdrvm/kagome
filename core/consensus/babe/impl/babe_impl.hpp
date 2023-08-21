/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_IMPL_HPP
#define KAGOME_BABE_IMPL_HPP

#include "consensus/babe/babe.hpp"

#include "application/app_configuration.hpp"
#include "clock/timer.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "parachain/availability/bitfield/store.hpp"
#include "parachain/backing/store.hpp"
#include "primitives/block.hpp"
#include "primitives/event_types.hpp"
#include "primitives/inherent_data.hpp"
#include "telemetry/service.hpp"

namespace kagome::application {
  class AppStateManager;
}  // namespace kagome::application

namespace kagome::authorship {
  class Proposer;
}  // namespace kagome::authorship

namespace kagome::blockchain {
  class DigestTracker;
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::consensus::babe {
  class BabeLottery;
  class BabeUtil;
  class BlockExecutor;
}  // namespace kagome::consensus::babe

namespace kagome::consensus::grandpa {
  class GrandpaDigestObserver;
  struct JustificationObserver;
}  // namespace kagome::consensus::grandpa

namespace kagome::crypto {
  class Hasher;
  class Sr25519Provider;
  class SessionKeys;
}  // namespace kagome::crypto

namespace kagome::dispute {
  class DisputeCoordinator;
}

namespace kagome::network {
  class Synchronizer;
  class BlockAnnounceTransmitter;
  class WarpSync;
  class WarpProtocol;
}  // namespace kagome::network

namespace kagome::runtime {
  class OffchainWorkerApi;
  class Core;
}  // namespace kagome::runtime

namespace kagome::consensus::babe {
  class BabeConfigRepository;
  class ConsistencyKeeper;
}  // namespace kagome::consensus::babe

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::consensus::babe {

  inline const auto kTimestampId =
      primitives::InherentIdentifier::fromString("timstap0").value();
  inline const auto kBabeSlotId =
      primitives::InherentIdentifier::fromString("babeslot").value();
  inline const auto kParachainId =
      primitives::InherentIdentifier::fromString("parachn0").value();
  inline const auto kUnsupportedInherentId_uncles00 =
      primitives::InherentIdentifier::fromString("uncles00").value();

  /// The maximum allowed number of slots past the expected slot as a delay for
  /// block production. This is an intentional relaxation of block dropping algo
  static constexpr auto kMaxBlockSlotsOvertime = 2;

  class BabeImpl : public Babe, public std::enable_shared_from_this<BabeImpl> {
   public:
    /**
     * Create an instance of Babe implementation
     */
    BabeImpl(
        const application::AppConfiguration &app_config,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<BabeLottery> lottery,
        std::shared_ptr<BabeConfigRepository> babe_config_repo,
        std::shared_ptr<authorship::Proposer> proposer,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<network::BlockAnnounceTransmitter>
            block_announce_transmitter,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
        std::shared_ptr<crypto::SessionKeys> session_keys,
        std::shared_ptr<clock::SystemClock> clock,
        std::shared_ptr<crypto::Hasher> hasher,
        std::unique_ptr<clock::Timer> timer,
        std::shared_ptr<blockchain::DigestTracker> digest_tracker,
        std::shared_ptr<network::WarpSync> warp_sync,
        LazySPtr<network::WarpProtocol> warp_protocol,
        std::shared_ptr<consensus::grandpa::JustificationObserver>
            justification_observer,
        std::shared_ptr<network::Synchronizer> synchronizer,
        std::shared_ptr<BabeUtil> babe_util,
        std::shared_ptr<parachain::BitfieldStore> bitfield_store,
        std::shared_ptr<parachain::BackingStore> backing_store,
        primitives::events::StorageSubscriptionEnginePtr storage_sub_engine,
        primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
        std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
        std::shared_ptr<runtime::Core> core,
        std::shared_ptr<ConsistencyKeeper> consistency_keeper,
        std::shared_ptr<storage::trie::TrieStorage> trie_storage,
        primitives::events::BabeStateSubscriptionEnginePtr
            babe_status_observable,
        std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator);

    ~BabeImpl() override = default;

    /** @see AppStateManager::takeControl */
    bool prepare();

    /** @see AppStateManager::takeControl */
    bool start();

    void runEpoch();

    State getCurrentState() const override;

    void onBlockAnnounceHandshake(
        const libp2p::peer::PeerId &peer_id,
        const network::BlockAnnounceHandshake &handshake) override;

    void onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                         const network::BlockAnnounce &announce) override;

    bool wasSynchronized() const override;

   private:
    /**
     * Warp sync from `peer_id` if `block_number`.
     * @return false if can't warp sync
     */
    bool warpSync(const libp2p::peer::PeerId &peer_id,
                  primitives::BlockNumber block_number);

    bool updateSlot(BabeTimePoint now);

    void startCatchUp(const libp2p::peer::PeerId &peer_id,
                      const primitives::BlockInfo &target_block);

    void onCaughtUp(const primitives::BlockInfo &block);

    void onSynchronized();

    void startStateSyncing(const libp2p::peer::PeerId &peer_id);

    void runSlot();

    /**
     * Process the current Babe slot
     */
    void processSlot(clock::SystemClock::TimePoint slot_timestamp);

    /**
     * Gather block and broadcast it
     * @param slot_type - kind of slot to compose correct block header
     * @param output - VRF proof if required (determined by slot_type)
     * @param authority_index - this node index in epoch authorities list
     */
    void processSlotLeadership(
        SlotType slot_type,
        clock::SystemClock::TimePoint slot_timestamp,
        std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
        primitives::AuthorityIndex authority_index);

    void changeLotteryEpoch(
        const EpochDescriptor &epoch,
        primitives::AuthorityIndex authority_index,
        const primitives::BabeConfiguration &babe_config) const;

    outcome::result<primitives::PreRuntime> babePreDigest(
        SlotType slot_type,
        std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
        primitives::AuthorityIndex authority_index) const;

    outcome::result<primitives::Seal> sealBlock(
        const primitives::Block &block) const;

    application::AppConfiguration::SyncMethod sync_method_;
    bool app_config_validator_;
    std::shared_ptr<application::AppStateManager> app_state_manager_;
    std::shared_ptr<BabeLottery> lottery_;
    std::shared_ptr<BabeConfigRepository> babe_config_repo_;
    std::shared_ptr<authorship::Proposer> proposer_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<network::BlockAnnounceTransmitter>
        block_announce_transmitter_;
    std::shared_ptr<crypto::Sr25519Keypair> keypair_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::unique_ptr<clock::Timer> timer_;
    std::shared_ptr<blockchain::DigestTracker> digest_tracker_;
    std::shared_ptr<network::WarpSync> warp_sync_;
    LazySPtr<network::WarpProtocol> warp_protocol_;
    std::shared_ptr<consensus::grandpa::JustificationObserver>
        justification_observer_;
    std::shared_ptr<network::Synchronizer> synchronizer_;
    std::shared_ptr<BabeUtil> babe_util_;
    std::shared_ptr<parachain::BitfieldStore> bitfield_store_;
    std::shared_ptr<parachain::BackingStore> backing_store_;
    primitives::events::StorageSubscriptionEnginePtr storage_sub_engine_;
    primitives::events::ChainSubscriptionEnginePtr chain_events_engine_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;
    std::optional<primitives::Version> actual_runtime_version_;
    std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api_;
    std::shared_ptr<runtime::Core> runtime_core_;
    std::shared_ptr<ConsistencyKeeper> consistency_keeper_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;
    primitives::events::BabeStateSubscriptionEnginePtr babe_status_observable_;
    std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator_;

    bool warp_sync_busy_ = false;

    State current_state_{State::WAIT_REMOTE_STATUS};

    bool was_synchronized_{false};

    std::atomic_bool active_{false};

    EpochDescriptor current_epoch_;

    BabeSlotNumber current_slot_{};

    primitives::BlockInfo best_block_{};

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_is_major_syncing_;
    metrics::Gauge *metric_is_relaychain_validator_;

    log::Logger log_;
    telemetry::Telemetry telemetry_;  // telemetry
  };
}  // namespace kagome::consensus::babe

#endif  // KAGOME_BABE_IMPL_HPP
