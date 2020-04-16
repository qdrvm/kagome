/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_BABE_IMPL_HPP
#define KAGOME_BABE_IMPL_HPP

#include "consensus/babe.hpp"

#include <memory>

#include <boost/asio/basic_waitable_timer.hpp>
#include <outcome/outcome.hpp>
#include "authorship/proposer.hpp"
#include "blockchain/block_tree.hpp"
#include "clock/timer.hpp"
#include "common/logger.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "consensus/babe/babe_synchronizer.hpp"
#include "consensus/babe/epoch_storage.hpp"
#include "consensus/validation/block_validator.hpp"
#include "crypto/hasher.hpp"
#include "crypto/sr25519_types.hpp"
#include "network/babe_gossiper.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/common.hpp"
#include "runtime/babe_api.hpp"
#include "runtime/core.hpp"

namespace kagome::consensus {

  enum class BabeState {
    WAIT_BLOCK,   // Node is just executed and waits for the new block to sync
                  // missing blocks
    CATCHING_UP,  // Node received first block announce and started fetching
                  // blocks between announced one and the latest finalized one
    NEED_SLOT_TIME,  // Missing blocks were received, now slot time should be
                     // calculated
    SYNCHRONIZED  // All missing blocks were received and applied, slot time was
                  // calculated, current peer can start block production
  };

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
    BabeImpl(std::shared_ptr<BabeLottery> lottery,
             std::shared_ptr<BabeSynchronizer> babe_synchronizer,
             std::shared_ptr<BlockValidator> block_validator,
             std::shared_ptr<EpochStorage> epoch_storage,
             std::shared_ptr<runtime::BabeApi> babe_api,
             std::shared_ptr<runtime::Core> core,
             std::shared_ptr<authorship::Proposer> proposer,
             std::shared_ptr<blockchain::BlockTree> block_tree,
             std::shared_ptr<network::BabeGossiper> gossiper,
             crypto::SR25519Keypair keypair,
             std::shared_ptr<clock::SystemClock> clock,
             std::shared_ptr<crypto::Hasher> hasher,
             std::unique_ptr<clock::Timer> timer);

    ~BabeImpl() override = default;

    void runGenesisEpoch() override;

    void runEpoch(Epoch epoch,
                  BabeTimePoint starting_slot_finish_time) override;

    void onBlockAnnounce(const network::BlockAnnounce &announce) override;

    BabeMeta getBabeMeta() const;

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

    /**
     * Processes next header: if header is observed first it is added to the
     * storage, handler is invoked. Synchronization of blocks between new one
     * and the current best one is launched if required
     * @param header new header that we received and trying to process
     * @param new_block_handler invoked on new header if it is observed first
     * time
     */
    void processNextBlock(
        const primitives::BlockHeader &header,
        const std::function<void(const primitives::BlockHeader &)>
            &new_block_handler);

    outcome::result<primitives::PreRuntime> babePreDigest(
        const crypto::VRFOutput &output,
        primitives::AuthorityIndex authority_index) const;

    primitives::Seal sealBlock(const primitives::Block &block) const;

    /**
     * To be called if we are far behind other nodes to skip some slots and
     * finally synchronize with the network
     */
    void synchronizeSlots(const primitives::BlockHeader &new_header);

    /**
     * Synchronize all missing blocks between the last finalized and the new one
     * @param new_header header defining new block
     * @param next action after the sync is done
     */
    void requestBlocks(const primitives::BlockHeader &new_header,
                       std::function<void()> next);

    /**
     * Synchronize all missing blocks between provided blocks (from and to)
     * @param from starting block of syncing blocks
     * @param to last block of syncing block
     * @param next action after the sync is done
     */
    void requestBlocks(const primitives::BlockId &from,
                       const primitives::BlockHash &to,
                       std::function<void()> next);

    // should only be invoked when parent of block exists
    outcome::result<void> applyBlock(const primitives::Block &block);

   private:
    std::shared_ptr<BabeLottery> lottery_;
    std::shared_ptr<BabeSynchronizer> babe_synchronizer_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<EpochStorage> epoch_storage_;
    std::shared_ptr<runtime::BabeApi> babe_api_;
    std::shared_ptr<runtime::Core> core_;
    std::shared_ptr<authorship::Proposer> proposer_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<network::BabeGossiper> gossiper_;
    crypto::SR25519Keypair keypair_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::unique_ptr<clock::Timer> timer_;

    primitives::BabeConfiguration genesis_configuration_;

    BabeState current_state_{BabeState::WAIT_BLOCK};

    Epoch current_epoch_;

    /// Estimates of the first block production slot time. Input for the median
    /// algorithm
    std::vector<BabeTimePoint> first_slot_times_{};

    /// Number of blocks we need to use in median algorithm to get the slot time
    const uint32_t kSlotTail = 30;

    BabeSlotNumber current_slot_{};
    BabeLottery::SlotsLeadership slots_leadership_;
    BabeTimePoint next_slot_finish_time_;

    common::Logger log_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_IMPL_HPP
