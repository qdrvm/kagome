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
#include "common/logger.hpp"
#include "consensus/authority/authority_update_observer.hpp"
#include "consensus/babe/babe_gossiper.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "consensus/babe/epoch_storage.hpp"
#include "consensus/babe/impl/block_executor.hpp"
#include "consensus/babe/types/slots_strategy.hpp"
#include "crypto/hasher.hpp"
#include "crypto/sr25519_provider.hpp"
#include "crypto/sr25519_types.hpp"
#include "outcome/outcome.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/common.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::consensus {

  inline const auto kTimestampId =
      primitives::InherentIdentifier::fromString("timstap0").value();
  inline const auto kBabeSlotId =
      primitives::InherentIdentifier::fromString("babeslot").value();

  class BabeImpl : public Babe, public std::enable_shared_from_this<BabeImpl> {
   public:
    /**
     * Create an instance of Babe implementation
     * @param lottery - implementation of Babe Lottery
     * @param proposer - block proposer
     * @param block_tree - tree of the blocks
     * @param gossiper of this consensus
     * @param keypair - SR25519 keypair of this node
     * @param authority_index of this node
     * @param clock to measure time
     * @param hasher to take hashes
     * @param timer to be used by the implementation; the recommended one is
     * kagome::clock::BasicWaitableTimer
     * @param event_bus to deliver events over
     */
    BabeImpl(std::shared_ptr<application::AppStateManager> app_state_manager,
             std::shared_ptr<BabeLottery> lottery,
             std::shared_ptr<BlockExecutor> block_executor,
             std::shared_ptr<storage::trie::TrieStorage> trie_db,
             std::shared_ptr<EpochStorage> epoch_storage,
             std::shared_ptr<primitives::BabeConfiguration> configuration,
             std::shared_ptr<authorship::Proposer> proposer,
             std::shared_ptr<blockchain::BlockTree> block_tree,
             std::shared_ptr<BabeGossiper> gossiper,
             std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
             crypto::Sr25519Keypair keypair,
             std::shared_ptr<clock::SystemClock> clock,
             std::shared_ptr<crypto::Hasher> hasher,
             std::unique_ptr<clock::Timer> timer,
             std::shared_ptr<authority::AuthorityUpdateObserver>
                 authority_update_observer,
             SlotsStrategy slots_calculation_strategy);

    ~BabeImpl() override = default;

    bool start();

    void setExecutionStrategy(ExecutionStrategy strategy) override {
      execution_strategy_ = strategy;
    }

    void runEpoch(Epoch epoch,
                  BabeTimePoint starting_slot_finish_time) override;

    State getCurrentState() const override;

    void onBlockAnnounce(const network::BlockAnnounce &announce) override;

    void doOnSynchronized(std::function<void()> handler) override;

   private:
    /**
     * Run the next Babe slot
     */
    void runSlot();

    /**
     * Finish the current Babe slot
     */
    void finishSlot();

    /**
     * Gather the block and broadcast it
     * @param output that we are the leader of this slot
     */
    void processSlotLeadership(const crypto::VRFOutput &output);

    /**
     * Finish the Babe epoch
     */
    void finishEpoch();

    BabeLottery::SlotsLeadership getEpochLeadership(const Epoch &epoch) const;

    outcome::result<primitives::PreRuntime> babePreDigest(
        const crypto::VRFOutput &output,
        primitives::AuthorityIndex authority_index) const;

    outcome::result<primitives::Seal> sealBlock(
        const primitives::Block &block) const;

    //--------------------------------------------------------------------------
    /**
     * Stores first production slot time estimate to the collection, to take
     * their median later
     * @param observed_slot Slot number of current block
     * @param first_production_slot_number Slot number of the first production slot
     */
    void storeFirstSlotTimeEstimate(
        BabeSlotNumber observed_slot,
        BabeSlotNumber first_production_slot_number);

    /**
     * Get first production slot time
     * @return median of first production slot times estimates
     */
    BabeTimePoint getFirstSlotTimeEstimate() const;

    /**
     * Create first epoch where current node will produce blocks
     * @param first_slot_time_estimate when first slot should be launched
     * @param first_production_slot_number
     * @param new_epoch_descriptor structure with the authorities and randomness of the new epoch
     * @return first production epoch structure
     */
    Epoch prepareFirstEpoch(
        BabeTimePoint first_slot_time_estimate,
        BabeSlotNumber first_production_slot_number,
        const NextEpochDescriptor &new_epoch_descriptor) const;
    //--------------------------------------------------------------------------

    Epoch prepareFirstEpochUnixTime(LastEpochDescriptor last_known_epoch,
                                    BabeSlotNumber first_production_slot) const;

    /**
     * To be called if we are far behind other nodes to skip some slots and
     * finally synchronize with the network
     */
    void synchronizeSlots(const primitives::BlockHeader &new_header);

   private:
    std::shared_ptr<application::AppStateManager> app_state_manager_;
    std::shared_ptr<BabeLottery> lottery_;
    std::shared_ptr<BlockExecutor> block_executor_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;
    std::shared_ptr<EpochStorage> epoch_storage_;
    std::shared_ptr<primitives::BabeConfiguration> genesis_configuration_;
    std::shared_ptr<authorship::Proposer> proposer_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<BabeGossiper> gossiper_;
    crypto::Sr25519Keypair keypair_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
    std::unique_ptr<clock::Timer> timer_;
    std::shared_ptr<authority::AuthorityUpdateObserver>
        authority_update_observer_;
    const SlotsStrategy slots_calculation_strategy_;

    State current_state_{State::WAIT_BLOCK};

    Epoch current_epoch_;

    /// Estimates of the first block production slot time. Input for the median
    /// algorithm
    std::vector<BabeTimePoint> first_slot_times_{};

    /// Number of blocks we need to use in median algorithm to get the slot time
    const uint32_t kSlotTail = 30;

    BabeSlotNumber current_slot_{};
    BabeLottery::SlotsLeadership slots_leadership_;
    BabeTimePoint next_slot_finish_time_;

    boost::optional<ExecutionStrategy> execution_strategy_;

    std::function<void()> on_synchronized_;

    common::Logger log_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_IMPL_HPP
