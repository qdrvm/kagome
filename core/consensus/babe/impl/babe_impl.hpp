/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_IMPL_HPP
#define KAGOME_BABE_IMPL_HPP

#include "consensus/babe/babe.hpp"

#include "clock/timer.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "primitives/block.hpp"
#include "primitives/event_types.hpp"
#include "primitives/inherent_data.hpp"
#include "telemetry/service.hpp"

namespace kagome::application {
  class AppConfiguration;
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
}

namespace kagome::crypto {
  class Hasher;
  class Sr25519Provider;
}  // namespace kagome::crypto

namespace kagome::network {
  class Synchronizer;
  class BlockAnnounceTransmitter;
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
    BabeImpl(const application::AppConfiguration &app_config,
             std::shared_ptr<application::AppStateManager> app_state_manager,
             std::shared_ptr<BabeLottery> lottery,
             std::shared_ptr<BabeConfigRepository> babe_config_repo,
             std::shared_ptr<authorship::Proposer> proposer,
             std::shared_ptr<blockchain::BlockTree> block_tree,
             std::shared_ptr<network::BlockAnnounceTransmitter>
                 block_announce_transmitter,
             std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
             const std::shared_ptr<crypto::Sr25519Keypair> &keypair,
             std::shared_ptr<clock::SystemClock> clock,
             std::shared_ptr<crypto::Hasher> hasher,
             std::unique_ptr<clock::Timer> timer,
             std::shared_ptr<blockchain::DigestTracker> digest_tracker,
             std::shared_ptr<network::Synchronizer> synchronizer,
             std::shared_ptr<BabeUtil> babe_util,
             primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
             std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
             std::shared_ptr<runtime::Core> core,
             std::shared_ptr<ConsistencyKeeper> consistency_keeper,
             std::shared_ptr<storage::trie::TrieStorage> trie_storage);

    ~BabeImpl() override = default;

    /** @see AppStateManager::takeControl */
    bool prepare();

    /** @see AppStateManager::takeControl */
    bool start();

    /** @see AppStateManager::takeControl */
    void stop();

    void runEpoch(EpochDescriptor epoch) override;

    State getCurrentState() const override;

    void onRemoteStatus(const libp2p::peer::PeerId &peer_id,
                        const network::Status &status) override;

    void onBlockAnnounce(const libp2p::peer::PeerId &peer_id,
                         const network::BlockAnnounce &announce) override;

    void onSynchronized() override;

    bool wasSynchronized() const override;

   private:
    outcome::result<EpochDescriptor> getInitialEpochDescriptor();

    void adjustEpochDescriptor();

    void startCatchUp(const libp2p::peer::PeerId &peer_id,
                      const primitives::BlockInfo &target_block);

    void startStateSyncing(const libp2p::peer::PeerId &peer_id);

    void runSlot();

    /**
     * Process the current Babe slot
     */
    void processSlot();

    /**
     * Gather block and broadcast it
     * @param slot_type - kind of slot to compose correct block header
     * @param output - VRF proof if required (determined by slot_type)
     * @param authority_index - this node index in epoch authorities list
     */
    void processSlotLeadership(
        SlotType slot_type,
        std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
        primitives::AuthorityIndex authority_index);

    /**
     * Finish the Babe epoch
     */
    void startNextEpoch();

    void changeLotteryEpoch(
        const EpochDescriptor &epoch,
        std::shared_ptr<const primitives::BabeConfiguration> babe_config) const;

    outcome::result<primitives::PreRuntime> babePreDigest(
        SlotType slot_type,
        std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
        primitives::AuthorityIndex authority_index) const;

    outcome::result<primitives::Seal> sealBlock(
        const primitives::Block &block) const;

    const application::AppConfiguration &app_config_;
    std::shared_ptr<BabeLottery> lottery_;
    std::shared_ptr<BabeConfigRepository> babe_config_repo_;
    std::shared_ptr<authorship::Proposer> proposer_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<network::BlockAnnounceTransmitter>
        block_announce_transmitter_;
    const std::shared_ptr<crypto::Sr25519Keypair> &keypair_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
    std::unique_ptr<clock::Timer> timer_;
    std::shared_ptr<blockchain::DigestTracker> digest_tracker_;
    std::shared_ptr<network::Synchronizer> synchronizer_;
    std::shared_ptr<BabeUtil> babe_util_;
    primitives::events::ChainSubscriptionEnginePtr chain_events_engine_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;
    std::optional<primitives::Version> actual_runtime_version_;
    std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api_;
    std::shared_ptr<runtime::Core> runtime_core_;
    std::shared_ptr<ConsistencyKeeper> consistency_keeper_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;

    State current_state_{State::WAIT_REMOTE_STATUS};

    bool was_synchronized_{false};

    std::atomic_bool active_{false};

    EpochDescriptor current_epoch_;

    BabeSlotNumber current_slot_{};

    primitives::BlockInfo best_block_{};

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Histogram *metric_block_proposal_time_;

    log::Logger log_;
    telemetry::Telemetry telemetry_;  // telemetry
  };
}  // namespace kagome::consensus::babe

#endif  // KAGOME_BABE_IMPL_HPP
