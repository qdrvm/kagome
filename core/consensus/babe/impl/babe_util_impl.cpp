/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_util_impl.hpp"
#include "blockchain/block_storage_error.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"

namespace kagome::consensus {

  BabeUtilImpl::BabeUtilImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      const BabeClock &clock)
      : babe_configuration_(std::move(babe_configuration)),
        block_tree_(std::move(block_tree)),
        clock_(clock),
        log_{log::createLogger("BabeUtil", "babe")} {
    BOOST_ASSERT(babe_configuration_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT_MSG(babe_configuration_->epoch_length,
                     "Epoch length must be non zero");

    app_state_manager->atPrepare([&] { return prepare(); });
  }

  bool BabeUtilImpl::prepare() {
    auto res = getInitialEpochDescriptor();
    if (res.has_error()) {
      SL_CRITICAL(log_,
                  "Can't get initial epoch descriptor: {}",
                  res.error().message());
      return false;
    }

    last_epoch_.emplace(res.value());
    return true;
  }

  outcome::result<EpochDescriptor> BabeUtilImpl::getInitialEpochDescriptor() {
    // First, look up slot number of block number 1
    auto first_block_header_res = block_tree_->getBlockHeader(1);
    if (first_block_header_res
        == outcome::failure(
            blockchain::BlockStorageError::HEADER_DOES_NOT_EXIST)) {
      EpochDescriptor epoch_descriptor{
          .epoch_number = 0,
          .start_slot =
              static_cast<BabeSlotNumber>(clock_.now().time_since_epoch()
                                          / babe_configuration_->slot_duration)
              + 1};
      return outcome::success(epoch_descriptor);
    }

    OUTCOME_TRY(first_block_header, first_block_header_res);
    auto babe_digest_res = getBabeDigests(first_block_header);
    BOOST_ASSERT_MSG(babe_digest_res.has_value(),
                     "Any non genesis block must be contain babe digest");
    auto first_slot_number = babe_digest_res.value().second.slot_number;

    // Second, look up slot number of best block
    auto best_block_number = block_tree_->deepestLeaf().number;
    auto best_block_header_res = block_tree_->getBlockHeader(best_block_number);
    BOOST_ASSERT_MSG(best_block_header_res.has_value(),
                     "Best block must be known whenever");
    const auto &best_block_header = best_block_header_res.value();
    babe_digest_res = getBabeDigests(best_block_header);
    BOOST_ASSERT_MSG(babe_digest_res.has_value(),
                     "Any non genesis block must be contain babe digest");
    auto last_slot_number = babe_digest_res.value().second.slot_number;

    BOOST_ASSERT_MSG(last_slot_number >= first_slot_number,
                     "Non genesis slot must not be less then genesis slot");

    // Now we have all to get epoch number
    EpochNumber epoch_number = (last_slot_number - first_slot_number)
                               / babe_configuration_->epoch_length;

    EpochDescriptor epoch_descriptor{
        .epoch_number = epoch_number,
        .start_slot = first_slot_number
                      + epoch_number * babe_configuration_->epoch_length};

    return outcome::success(epoch_descriptor);
  }

  BabeSlotNumber BabeUtilImpl::getCurrentSlot() const {
    return static_cast<BabeSlotNumber>(clock_.now().time_since_epoch()
                                       / babe_configuration_->slot_duration);
  }

  BabeTimePoint BabeUtilImpl::slotStartTime(BabeSlotNumber slot) const {
    return clock_.zero() + slot * babe_configuration_->slot_duration;
  }

  BabeDuration BabeUtilImpl::remainToStartOfSlot(BabeSlotNumber slot) const {
    auto deadline = slotStartTime(slot);
    auto now = clock_.now();
    if (deadline > now) {
      return deadline - now;
    }
    return BabeDuration{};
  }

  BabeTimePoint BabeUtilImpl::slotFinishTime(BabeSlotNumber slot) const {
    return slotStartTime(slot + 1);
  }

  BabeDuration BabeUtilImpl::remainToFinishOfSlot(BabeSlotNumber slot) const {
    return remainToStartOfSlot(slot + 1);
  }

  BabeDuration BabeUtilImpl::slotDuration() const {
    return babe_configuration_->slot_duration;
  }

  BabeSlotNumber BabeUtilImpl::getGenesisSlotNumber() {
    if (genesis_slot_number_.has_value()) {
      return genesis_slot_number_.value();
    }

    return getCurrentSlot();
  }

  EpochNumber BabeUtilImpl::slotToEpoch(BabeSlotNumber slot) const {
    auto genesis_slot_number =
        const_cast<BabeUtilImpl &>(*this).getGenesisSlotNumber();
    if (slot > genesis_slot_number) {
      return (slot - genesis_slot_number) / babe_configuration_->epoch_length;
    }
    return 0;
  }

  BabeSlotNumber BabeUtilImpl::slotInEpoch(BabeSlotNumber slot) const {
    auto genesis_slot_number =
        const_cast<BabeUtilImpl &>(*this).getGenesisSlotNumber();
    if (slot > genesis_slot_number) {
      return (slot - genesis_slot_number) % babe_configuration_->epoch_length;
    }
    return 0;
  }

  outcome::result<void> BabeUtilImpl::setLastEpoch(
      const EpochDescriptor &epoch_descriptor) {
    if (not genesis_slot_number_.has_value()) {
      genesis_slot_number_.emplace(epoch_descriptor.start_slot
                                   - epoch_descriptor.epoch_number
                                         * babe_configuration_->epoch_length);
    }

    last_epoch_.emplace(epoch_descriptor);
    return outcome::success();
  }

  const EpochDescriptor &BabeUtilImpl::getLastEpoch() const {
    BOOST_ASSERT_MSG(last_epoch_.has_value(),
                     "Last epoch descriptor must be defined before");
    return last_epoch_.value();
  }

}  // namespace kagome::consensus
