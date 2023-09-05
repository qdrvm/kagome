/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/slots_util_impl.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/consensus_selector.hpp"
#include "consensus/timeline/impl/timeline_error.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome ::consensus {

  SlotsUtilImpl::SlotsUtilImpl(
      application::AppStateManager &app_state_manager,
      std::shared_ptr<storage::SpacedStorage> persistent_storage,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<ConsensusSelector> consensus_selector)
      : log_(log::createLogger("SlotsUtil", "timeline")),
        persistent_storage_(
            persistent_storage->getSpace(storage::Space::kDefault)),
        block_tree_(std::move(block_tree)),
        consensus_selector_(std::move(consensus_selector)) {
    BOOST_ASSERT(persistent_storage_);
    BOOST_ASSERT(block_tree_);

    app_state_manager.takeControl(*this);
  }

  bool SlotsUtilImpl::prepare() {
    auto finalized = block_tree_->getLastFinalized();
    auto consensus = consensus_selector_->getProductionConsensus(finalized);
    std::tie(slot_duration_, epoch_length_) = consensus->getTimings();
    return true;
  }

  Duration SlotsUtilImpl::slotDuration() const {
    BOOST_ASSERT_MSG(slot_duration_ != Duration::zero(),
                     "Slot duration is not initialized");
    return slot_duration_;
  }

  EpochLength SlotsUtilImpl::epochLength() const {
    BOOST_ASSERT_MSG(epoch_length_ != 0, "Epoch length is not initialized");
    return epoch_length_;
  }

  SlotNumber SlotsUtilImpl::timeToSlot(TimePoint time) const {
    return static_cast<SlotNumber>(time.time_since_epoch() / slotDuration());
  }

  TimePoint SlotsUtilImpl::slotStartTime(SlotNumber slot) const {
    return TimePoint{} + slot * slotDuration();
  }

  TimePoint SlotsUtilImpl::slotFinishTime(SlotNumber slot) const {
    return slotStartTime(slot + 1);
  }

  outcome::result<EpochNumber> SlotsUtilImpl::slotToEpoch(
      const primitives::BlockInfo &parent_info, SlotNumber slot) const {
    [[unlikely]] if (parent_info.number == 0) { return 0; }
    OUTCOME_TRY(slot1, getFirstBlockSlotNumber(parent_info));
    if (slot < slot1) {
      return TimelineError::SLOT_BEFORE_GENESIS;
    }
    auto slots = slot - slot1;
    return slots / epochLength();
  }

  outcome::result<SlotNumber> SlotsUtilImpl::getFirstBlockSlotNumber(
      const primitives::BlockInfo &parent_info) const {
    [[likely]] if (first_block_slot_number_.has_value()) {
      return first_block_slot_number_.value();
    }

    // If the first block has finalized, that get slot from it
    auto finalized = block_tree_->getLastFinalized();
    if (finalized.number != 0) {
      OUTCOME_TRY(hash1, block_tree_->getBlockHash(1));
      if (hash1) {
        auto consensus =
            consensus_selector_->getProductionConsensus(parent_info);
        OUTCOME_TRY(header1, block_tree_->getBlockHeader(*hash1));
        OUTCOME_TRY(slot1, consensus->getSlot(header1));
        return first_block_slot_number_.emplace(slot1);
      }
    }

    std::optional<SlotNumber> slot1;

    OUTCOME_TRY(parent, block_tree_->getBlockHeader(parent_info.hash));

    // Trying to get slot from consensus (internally it happens by runtime call)
    // {
    //   auto consensus = consensus_selector_->getProductionConsensus(
    //      parent_info);
    //   // TODO consensus->getTheFirstSlot();
    //   if (trie_storage_->getEphemeralBatchAt(parent.state_root)) {
    //     if (auto epoch_res = babe_api_->next_epoch(parent_info.hash);
    //         epoch_res.has_value()) {
    //       const auto &epoch = epoch_res.value();
    //       slot1 = epoch.start_slot - epoch.epoch_index * epoch.duration;
    //     }
    //   }
    // }

    // If slot still unknown, getting slot from the first block which ancestor
    // of provided
    if (not slot1.has_value()) {
      auto header = parent;
      for (;;) {
        [[unlikely]] if (header.number == 1) {
          auto consensus =
              consensus_selector_->getProductionConsensus(parent_info);
          OUTCOME_TRY(slot, consensus->getSlot(header));
          slot1 = slot;
          break;
        }
        auto header_res = block_tree_->getBlockHeader(header.parent_hash);
        if (header_res.has_error()) {
          return header_res.as_failure();
        }
        header = std::move(header_res.value());
      }
    }

    if (finalized.number != 0
        and block_tree_->hasDirectChain(finalized, parent_info)) {
      first_block_slot_number_ = slot1;
      OUTCOME_TRY(persistent_storage_->put(
          storage::kBabeConfigRepositoryImplGenesisSlot,
          scale::encode(*slot1).value()));
    }

    return slot1.value();
  }

}  // namespace kagome::consensus
