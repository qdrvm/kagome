/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABEUTILIMPL
#define KAGOME_CONSENSUS_BABEUTILIMPL

#include "consensus/babe/babe_util.hpp"

#include "consensus/babe/common.hpp"
#include "primitives/babe_configuration.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::consensus {

  class BabeUtilImpl final : public BabeUtil {
   public:
    BabeUtilImpl(
        std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
        const BabeClock &clock);

    void syncEpoch(EpochDescriptor epoch_descriptor) override;

    BabeSlotNumber getCurrentSlot() const override;

    BabeTimePoint slotStartTime(BabeSlotNumber slot) const override;
    BabeDuration remainToStartOfSlot(BabeSlotNumber slot) const override;
    BabeTimePoint slotFinishTime(BabeSlotNumber slot) const override;
    BabeDuration remainToFinishOfSlot(BabeSlotNumber slot) const override;

    BabeDuration slotDuration() const override;

    EpochNumber slotToEpoch(BabeSlotNumber slot) const override;

    BabeSlotNumber slotInEpoch(BabeSlotNumber slot) const override;

   private:
    BabeSlotNumber getFirstBlockSlotNumber();

    std::shared_ptr<primitives::BabeConfiguration> babe_configuration_;
    const BabeClock &clock_;

    std::optional<BabeSlotNumber> first_block_slot_number_;
  };
}  // namespace kagome::consensus
#endif  // KAGOME_CONSENSUS_BABEUTILIMPL
