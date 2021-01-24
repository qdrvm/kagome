/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABEUTIL
#define KAGOME_CONSENSUS_BABEUTIL

#include "consensus/babe/common.hpp"
#include "consensus/babe/epoch_storage.hpp"
#include "consensus/babe/types/slots_strategy.hpp"
#include "primitives/babe_configuration.hpp"

namespace kagome::consensus {

  class BabeUtil final {
   public:
    BabeUtil(std::shared_ptr<EpochStorage> epoch_storage,
             std::shared_ptr<primitives::BabeConfiguration> config,
             const SlotsStrategy strategy,
             const BabeClock &clock) {
      if (auto res = epoch_storage->getLastEpoch(); res.has_value()) {
        auto epoch = res.value();
        genesis_slot_number_ =
            epoch.start_slot - epoch.epoch_number * config->epoch_length;
      } else {
        switch (strategy) {
          case SlotsStrategy::FromZero:
            genesis_slot_number_ = 0;
            break;
          case SlotsStrategy::FromUnixEpoch:
            auto ticks_since_epoch = clock.now().time_since_epoch().count();
            genesis_slot_number_ = static_cast<BabeSlotNumber>(
                ticks_since_epoch / config->slot_duration.count());
            break;
        }
      }
      epoch_length_ = config->epoch_length;
    }
    BabeUtil(BabeUtil &&) noexcept = delete;
    BabeUtil(const BabeUtil &) = delete;
    BabeUtil &operator=(BabeUtil &&) noexcept = delete;
    BabeUtil &operator=(BabeUtil const &) = delete;

    EpochIndex slotToEpoch(BabeSlotNumber slot) const {
      if (slot >= genesis_slot_number_) {
        return (slot - genesis_slot_number_) / epoch_length_;
      }
      return 0;
    }
    BabeSlotNumber slotInEpoch(BabeSlotNumber slot) const {
      if (slot >= genesis_slot_number_) {
        return (slot - genesis_slot_number_) % epoch_length_;
      }
      return 0;
    }

   private:
    BabeSlotNumber genesis_slot_number_;
    BabeSlotNumber epoch_length_;
  };
}  // namespace kagome::consensus
#endif  // KAGOME_CONSENSUS_BABEUTIL
