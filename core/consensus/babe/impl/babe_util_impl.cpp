/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_util_impl.hpp"

#include "consensus/babe/babe_config_repository.hpp"

namespace kagome::consensus {

  BabeUtilImpl::BabeUtilImpl(
      std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
      const BabeClock &clock)
      : babe_config_repo_(std::move(babe_config_repo)), clock_(clock) {
    BOOST_ASSERT(babe_config_repo_);
  }

  BabeSlotNumber BabeUtilImpl::syncEpoch(
      std::function<std::tuple<BabeSlotNumber, bool>()> &&f) {
    if (not is_first_block_finalized_) {
      auto [first_block_slot_number, is_first_block_finalized] = f();
      first_block_slot_number_.emplace(first_block_slot_number);
      is_first_block_finalized_ = is_first_block_finalized;
    }
    return first_block_slot_number_.value();
  }

  BabeSlotNumber BabeUtilImpl::getCurrentSlot() const {
    const auto &babe_config = babe_config_repo_->config();
    BOOST_ASSERT_MSG(
        babe_config.slot_duration > std::chrono::nanoseconds::zero(),
        "Slot duration must be non zero");
    return static_cast<BabeSlotNumber>(clock_.now().time_since_epoch()
                                       / babe_config.slot_duration);
  }

  BabeTimePoint BabeUtilImpl::slotStartTime(BabeSlotNumber slot) const {
    const auto &babe_config = babe_config_repo_->config();
    BOOST_ASSERT_MSG(
        babe_config.slot_duration > std::chrono::nanoseconds::zero(),
        "Slot duration must be non zero");
    return clock_.zero() + slot * babe_config.slot_duration;
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
    const auto &babe_config = babe_config_repo_->config();
    BOOST_ASSERT_MSG(
        babe_config.slot_duration > std::chrono::nanoseconds::zero(),
        "Slot duration must be non zero");
    return babe_config.slot_duration;
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
      const auto &babe_config = babe_config_repo_->config();
      BOOST_ASSERT_MSG(babe_config.epoch_length > 0,
                       "Epoch length must be non zero");
      return (slot - genesis_slot_number) / babe_config.epoch_length;
    }
    return 0;
  }

  BabeSlotNumber BabeUtilImpl::slotInEpoch(BabeSlotNumber slot) const {
    auto genesis_slot_number =
        const_cast<BabeUtilImpl &>(*this).getFirstBlockSlotNumber();
    if (slot > genesis_slot_number) {
      const auto &babe_config = babe_config_repo_->config();
      BOOST_ASSERT_MSG(babe_config.epoch_length > 0,
                       "Epoch length must be non zero");
      return (slot - genesis_slot_number) % babe_config.epoch_length;
    }
    return 0;
  }

}  // namespace kagome::consensus
