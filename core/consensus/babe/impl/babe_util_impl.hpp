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

namespace kagome::consensus::babe {
  class BabeConfigRepository;
}

namespace kagome::consensus {

  class BabeUtilImpl final : public BabeUtil {
   public:
    BabeUtilImpl(
        std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
        const BabeClock &clock);

    BabeSlotNumber syncEpoch(
        std::function<std::tuple<BabeSlotNumber, bool>()> &&f) override;

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

    std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo_;
    const BabeClock &clock_;

    std::optional<BabeSlotNumber> first_block_slot_number_;
    bool is_first_block_finalized_ = false;
  };
}  // namespace kagome::consensus
#endif  // KAGOME_CONSENSUS_BABEUTILIMPL
