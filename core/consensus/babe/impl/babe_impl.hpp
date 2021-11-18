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
#include "consensus/authority/authority_update_observer.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "consensus/babe/babe_util.hpp"
#include "consensus/babe/block_executor.hpp"
#include "crypto/hasher.hpp"
#include "crypto/sr25519_provider.hpp"
#include "crypto/sr25519_types.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "outcome/outcome.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/common.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::network {
  class Synchronizer;
  class BlockAnnounceTransmitter;
}  // namespace kagome::network

namespace kagome::runtime {
  class OffchainWorkerApi;
}  // namespace kagome::runtime

namespace kagome::consensus::babe {

  inline const auto kTimestampId =
      primitives::InherentIdentifier::fromString("timstap0").value();
  inline const auto kBabeSlotId =
      primitives::InherentIdentifier::fromString("babeslot").value();

  /// The maximum allowed number of slots past the expected slot as a delay for
  /// block production. This is an intentional relaxation of block dropping algo
  static constexpr auto kMaxBlockSlotsOvertime = 2;

  class BabeImpl : public Babe, public std::enable_shared_from_this<BabeImpl> {
   public:
    /**
     * Create an instance of Babe implementation
     */
    BabeImpl(std::shared_ptr<application::AppStateManager> app_state_manager,
             std::shared_ptr<BabeLottery> lottery,
             std::shared_ptr<storage::trie::TrieStorage> trie_db,
             std::shared_ptr<primitives::BabeConfiguration> configuration,
             std::shared_ptr<authorship::Proposer> proposer,
             std::shared_ptr<blockchain::BlockTree> block_tree,
             std::shared_ptr<network::BlockAnnounceTransmitter>
                 block_announce_transmitter,
             std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
             const std::shared_ptr<crypto::Sr25519Keypair> &keypair,
             std::shared_ptr<clock::SystemClock> clock,
             std::shared_ptr<crypto::Hasher> hasher,
             std::unique_ptr<clock::Timer> timer,
             std::shared_ptr<authority::AuthorityUpdateObserver>
                 authority_update_observer,
             std::shared_ptr<network::Synchronizer> synchronizer,
             std::shared_ptr<BabeUtil> babe_util,
             std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api);

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

   private:
    void startCatchUp(const libp2p::peer::PeerId &peer_id,
                      const primitives::BlockInfo &target_block);

    void runSlot();

    /**
     * Process the current Babe slot
     */
    void processSlot();

    /**
     * Gather the block and broadcast it
     * @param output that we are the leader of this slot
     */
    void processSlotLeadership(const crypto::VRFOutput &output);

    /**
     * Finish the Babe epoch
     */
    void startNextEpoch();

    void changeLotteryEpoch(const EpochDescriptor &epoch,
                            const primitives::AuthorityList &authorities,
                            const Randomness &randomness) const;

    outcome::result<primitives::PreRuntime> babePreDigest(
        const crypto::VRFOutput &output,
        primitives::AuthorityIndex authority_index) const;

    outcome::result<primitives::Seal> sealBlock(
        const primitives::Block &block) const;

   private:
    std::shared_ptr<BabeLottery> lottery_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;
    std::shared_ptr<primitives::BabeConfiguration> babe_configuration_;
    std::shared_ptr<authorship::Proposer> proposer_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<network::BlockAnnounceTransmitter>
        block_announce_transmitter_;
    const std::shared_ptr<crypto::Sr25519Keypair> &keypair_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
    std::unique_ptr<clock::Timer> timer_;
    std::shared_ptr<authority::AuthorityUpdateObserver>
        authority_update_observer_;
    std::shared_ptr<network::Synchronizer> synchronizer_;
    std::shared_ptr<BabeUtil> babe_util_;
    std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api_;

    State current_state_{State::WAIT_REMOTE_STATUS};

    std::atomic_bool active_{false};

    EpochDescriptor current_epoch_;

    BabeSlotNumber current_slot_{};

    primitives::BlockInfo best_block_{};

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Histogram *metric_block_proposal_time_;

    log::Logger log_;
  };
}  // namespace kagome::consensus::babe

#endif  // KAGOME_BABE_IMPL_HPP
