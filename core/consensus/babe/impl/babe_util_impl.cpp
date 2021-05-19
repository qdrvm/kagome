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
      const SlotsStrategy strategy,
      const BabeClock &clock)
      : babe_configuration_(std::move(babe_configuration)),
        storage_(std::move(storage)),
        strategy_(strategy),
        clock_(clock) {
    BOOST_ASSERT(babe_configuration_);
    BOOST_ASSERT(storage_);
    BOOST_ASSERT_MSG(babe_configuration_->epoch_length,
                     "Epoch length must be non zero");

    // If we have any known epoch, calculate from than. Otherwise we assume
    // that we are in the initial epoch and calculate with its rules.
    if (auto res = getLastEpoch(); res.has_value()) {
      auto epoch = res.value();
      genesis_slot_number_.emplace(epoch.start_slot
                                   - epoch.epoch_number
                                         * babe_configuration_->epoch_length);
    } else {
      switch (strategy_) {
        case SlotsStrategy::FromZero:
          genesis_slot_number_.emplace(0);
          break;

        case SlotsStrategy::FromUnixEpoch:
          // Will be initialized at epoch digest received
          break;
      }
    }
  }

  BabeSlotNumber BabeUtilImpl::getGenesisSlotNumber() {
    if (genesis_slot_number_.has_value()) {
      return genesis_slot_number_.value();
    }

    switch (strategy_) {
      case SlotsStrategy::FromZero:
        genesis_slot_number_.emplace(0);
        return genesis_slot_number_.value();

      case SlotsStrategy::FromUnixEpoch:
        auto ticks_since_epoch = clock_.now().time_since_epoch().count();
        return static_cast<BabeSlotNumber>(
            ticks_since_epoch / babe_configuration_->slot_duration.count());
    }
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
    OUTCOME_TRY(epoch_descriptor, storage_->get(key));
    return scale::decode<EpochDescriptor>(epoch_descriptor);
  }
}  // namespace kagome::consensus
