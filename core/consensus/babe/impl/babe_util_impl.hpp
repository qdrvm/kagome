/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABEUTILIMPL
#define KAGOME_CONSENSUS_BABEUTILIMPL

#include "consensus/babe/babe_util.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/babe/common.hpp"
#include "log/logger.hpp"
#include "primitives/babe_configuration.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::consensus {

  class BabeUtilImpl final : public BabeUtil {
   public:
    BabeUtilImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        const BabeClock &clock);

    BabeSlotNumber getCurrentSlot() const override;

    BabeTimePoint slotStartTime(BabeSlotNumber slot) const override;
    BabeDuration remainToStartOfSlot(BabeSlotNumber slot) const override;
    BabeTimePoint slotFinishTime(BabeSlotNumber slot) const override;
    BabeDuration remainToFinishOfSlot(BabeSlotNumber slot) const override;

    BabeDuration slotDuration() const override;

    EpochNumber slotToEpoch(BabeSlotNumber slot) const override;

    BabeSlotNumber slotInEpoch(BabeSlotNumber slot) const override;

    outcome::result<void> setLastEpoch(
        const EpochDescriptor &epoch_descriptor) override;

    const EpochDescriptor &getLastEpoch() const override;

   private:
    bool prepare();

    outcome::result<EpochDescriptor> getInitialEpochDescriptor();

    BabeSlotNumber getGenesisSlotNumber();

    std::shared_ptr<primitives::BabeConfiguration> babe_configuration_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    const BabeClock &clock_;

    std::optional<BabeSlotNumber> genesis_slot_number_;

    // optimization for storing in memory last epoch
    std::optional<EpochDescriptor> last_epoch_;

    log::Logger log_;
  };
}  // namespace kagome::consensus
#endif  // KAGOME_CONSENSUS_BABEUTILIMPL
