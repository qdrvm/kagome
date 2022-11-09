/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_IMPL_HPP
#define KAGOME_BABE_IMPL_HPP

#include "consensus/babe/babe.hpp"

#include <boost/asio/basic_waitable_timer.hpp>
#include <memory>

#include "application/app_state_manager.hpp"
#include "authorship/proposer.hpp"
#include "blockchain/block_tree.hpp"
#include "clock/timer.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "consensus/babe/babe_util.hpp"
#include "consensus/babe/block_executor.hpp"
#include "consensus/babe/types/slot.hpp"
#include "consensus/grandpa/grandpa_digest_observer.hpp"
#include "crypto/hasher.hpp"
#include "crypto/sr25519_provider.hpp"
#include "crypto/sr25519_types.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "outcome/outcome.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/common.hpp"
#include "primitives/event_types.hpp"
#include "storage/buffer_map_types.hpp"
#include "telemetry/service.hpp"

namespace kagome::application {
  class AppConfiguration;
}

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
        std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
        std::shared_ptr<authorship::Proposer> proposer,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<network::BlockAnnounceTransmitter>
            block_announce_transmitter,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
        const std::shared_ptr<crypto::Sr25519Keypair> &keypair,
        std::shared_ptr<clock::SystemClock> clock,
        std::shared_ptr<crypto::Hasher> hasher,
        std::unique_ptr<clock::Timer> timer,
        std::shared_ptr<consensus::grandpa::GrandpaDigestObserver>
            grandpa_digest_observer,
        std::shared_ptr<network::Synchronizer> synchronizer,
        std::shared_ptr<BabeUtil> babe_util,
        primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
        std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
        std::shared_ptr<runtime::Core> core,
        std::shared_ptr<babe::ConsistencyKeeper> consistency_keeper);

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
    std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo_;
    std::shared_ptr<authorship::Proposer> proposer_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<network::BlockAnnounceTransmitter>
        block_announce_transmitter_;
    const std::shared_ptr<crypto::Sr25519Keypair> &keypair_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
    std::unique_ptr<clock::Timer> timer_;
    std::shared_ptr<consensus::grandpa::GrandpaDigestObserver>
        grandpa_digest_observer_;
    std::shared_ptr<network::Synchronizer> synchronizer_;
    std::shared_ptr<BabeUtil> babe_util_;
    primitives::events::ChainSubscriptionEnginePtr chain_events_engine_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;
    std::optional<primitives::Version> actual_runtime_version_;
    std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api_;
    std::shared_ptr<runtime::Core> runtime_core_;
    std::shared_ptr<babe::ConsistencyKeeper> consistency_keeper_;

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
