/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/slots_util_impl.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/consensus_selector.hpp"
#include "consensus/timeline/impl/timeline_error.hpp"
#include "runtime/runtime_api/babe_api.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/spaced_storage.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome ::consensus {

  SlotsUtilImpl::SlotsUtilImpl(
      application::AppStateManager &app_state_manager,
      std::shared_ptr<storage::SpacedStorage> persistent_storage,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      const EpochTimings &timings,
      std::shared_ptr<ConsensusSelector> consensus_selector,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<runtime::BabeApi> babe_api)
      : log_(log::createLogger("SlotsUtil", "timeline")),
        persistent_storage_(
            persistent_storage->getSpace(storage::Space::kDefault)),
        block_tree_(std::move(block_tree)),
        timings_(timings),
        consensus_selector_(std::move(consensus_selector)),
        trie_storage_(std::move(trie_storage)),
        babe_api_(std::move(babe_api)) {
    BOOST_ASSERT(persistent_storage_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(trie_storage_);
    BOOST_ASSERT(babe_api_);

    app_state_manager.takeControl(*this);
  }

  bool SlotsUtilImpl::prepare() {
    if (auto slot1_res = persistent_storage_->tryGet(storage::kFirstBlockSlot);
        slot1_res.has_value()) {
      if (auto &slot1_opt = slot1_res.value(); slot1_opt.has_value()) {
        if (auto decode_res = scale::decode<SlotNumber>(slot1_opt.value());
            decode_res.has_value()) {
          first_block_slot_number_.emplace(decode_res.value());
        }
      }
    }

    return true;
  }

  Duration SlotsUtilImpl::slotDuration() const {
    BOOST_ASSERT_MSG(timings_, "Epoch timings are not initialized");
    return timings_.slot_duration;
  }

  EpochLength SlotsUtilImpl::epochLength() const {
    BOOST_ASSERT_MSG(timings_, "Epoch timings are not initialized");
    return timings_.epoch_length;
  }

  SlotNumber SlotsUtilImpl::timeToSlot(TimePoint time) const {
    return static_cast<SlotNumber>(time.time_since_epoch()
                                   / timings_.slot_duration);
  }

  TimePoint SlotsUtilImpl::slotStartTime(SlotNumber slot) const {
    return TimePoint{} + slot * timings_.slot_duration;
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
    return slots / timings_.epoch_length;
  }

  outcome::result<SlotNumber> SlotsUtilImpl::getFirstBlockSlotNumber(
      const primitives::BlockInfo &parent_info) const {
    [[likely]] if (first_block_slot_number_.has_value()) {
      return first_block_slot_number_.value();
    }

    std::optional<SlotNumber> slot1;

    // If the first block has finalized, that get slot from it
    auto finalized = block_tree_->getLastFinalized();
    if (finalized.number != 0) {
      OUTCOME_TRY(hash1, block_tree_->getBlockHash(1));
      if (hash1) {
        auto consensus =
            consensus_selector_->getProductionConsensus(parent_info);
        OUTCOME_TRY(header1, block_tree_->getBlockHeader(*hash1));
        OUTCOME_TRY(_slot1, consensus->getSlot(header1));
        slot1 = _slot1;
      }
    }

    OUTCOME_TRY(parent, block_tree_->getBlockHeader(parent_info.hash));

    // Trying to get slot from consensus (internally it happens by runtime call)
    if (not slot1.has_value()) {
      auto consensus = consensus_selector_->getProductionConsensus(parent_info);
      if (trie_storage_->getEphemeralBatchAt(parent.state_root)) {
        if (auto epoch_res = babe_api_->next_epoch(parent_info.hash);
            epoch_res.has_value()) {
          const auto &epoch = epoch_res.value();
          slot1 = epoch.start_slot - epoch.epoch_index * epoch.duration;
        }
      }
    }

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
      OUTCOME_TRY(persistent_storage_->put(storage::kFirstBlockSlot,
                                           scale::encode(*slot1).value()));
    }

    return slot1.value();
  }

}  // namespace kagome::consensus
