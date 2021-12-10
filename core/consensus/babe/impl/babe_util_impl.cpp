/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_util_impl.hpp"

#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::consensus {

  BabeUtilImpl::BabeUtilImpl(
      std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
      std::shared_ptr<storage::BufferStorage> storage,
      const BabeClock &clock)
      : babe_configuration_(std::move(babe_configuration)),
        storage_(std::move(storage)),
        clock_(clock) {
    BOOST_ASSERT(babe_configuration_);
    BOOST_ASSERT(storage_);
    BOOST_ASSERT_MSG(babe_configuration_->epoch_length,
                     "Epoch length must be non zero");

    // If we have any known epoch, calculate from than. Otherwise, we assume
    // that we are in the initial epoch and calculate with its rules.
    if (auto res = getLastEpoch(); res.has_value()) {
      auto epoch = res.value();
      genesis_slot_number_.emplace(epoch.start_slot
                                   - epoch.epoch_number
                                         * babe_configuration_->epoch_length);
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

    const auto &key = storage::kLastBabeEpochNumberLookupKey;
    auto val = common::Buffer{scale::encode(epoch_descriptor).value()};
    last_epoch_ = epoch_descriptor;
    return storage_->put(key, val);
  }

  outcome::result<EpochDescriptor> BabeUtilImpl::getLastEpoch() const {
    if (last_epoch_.has_value()) {
      return last_epoch_.value();
    }
    const auto &key = storage::kLastBabeEpochNumberLookupKey;
    OUTCOME_TRY(epoch_descriptor, storage_->load(key));
    auto &&res = scale::decode<EpochDescriptor>(epoch_descriptor);
    BOOST_ASSERT(res.has_value());
    return std::move(res);
  }
}  // namespace kagome::consensus
