/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_util_impl.hpp"

namespace kagome::consensus {

  BabeUtilImpl::BabeUtilImpl(
      std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
      const BabeClock &clock)
      : babe_configuration_(std::move(babe_configuration)), clock_(clock) {
    BOOST_ASSERT(babe_configuration_);
    BOOST_ASSERT_MSG(babe_configuration_->epoch_length,
                     "Epoch length must be non zero");
  }

  void BabeUtilImpl::syncEpoch(EpochDescriptor epoch_descriptor) {
    if (not first_block_slot_number_.has_value()) {
      first_block_slot_number_.emplace(
          epoch_descriptor.start_slot
          - epoch_descriptor.epoch_number * babe_configuration_->epoch_length);
    }
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

  BabeSlotNumber BabeUtilImpl::getFirstBlockSlotNumber() {
    if (first_block_slot_number_.has_value()) {
      return first_block_slot_number_.value();
    }

    return getCurrentSlot();
  }

  EpochNumber BabeUtilImpl::slotToEpoch(BabeSlotNumber slot) const {
    auto genesis_slot_number =
        const_cast<BabeUtilImpl &>(*this).getFirstBlockSlotNumber();
    if (slot > genesis_slot_number) {
      return (slot - genesis_slot_number) / babe_configuration_->epoch_length;
    }
    return 0;
  }

  BabeSlotNumber BabeUtilImpl::slotInEpoch(BabeSlotNumber slot) const {
    auto genesis_slot_number =
        const_cast<BabeUtilImpl &>(*this).getFirstBlockSlotNumber();
    if (slot > genesis_slot_number) {
      return (slot - genesis_slot_number) % babe_configuration_->epoch_length;
    }
    return 0;
  }

}  // namespace kagome::consensus
