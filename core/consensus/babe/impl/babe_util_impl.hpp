/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABEUTILIMPL
#define KAGOME_CONSENSUS_BABEUTILIMPL

#include "consensus/babe/babe_util.hpp"

#include "consensus/babe/common.hpp"
#include "consensus/babe/types/slots_strategy.hpp"
#include "primitives/babe_configuration.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::consensus {

  class BabeUtilImpl final : public BabeUtil {
   public:
    BabeUtilImpl(
        std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
        std::shared_ptr<storage::BufferStorage> storage,
        const SlotsStrategy strategy,
        const BabeClock &clock);

    EpochNumber slotToEpoch(BabeSlotNumber slot) const override;

    BabeSlotNumber slotInEpoch(BabeSlotNumber slot) const override;

    outcome::result<void> setLastEpoch(
        const EpochDescriptor &epoch_descriptor) override;

    outcome::result<EpochDescriptor> getLastEpoch() const override;

   private:
    std::shared_ptr<primitives::BabeConfiguration> babe_configuration_;
    std::shared_ptr<kagome::storage::BufferStorage> storage_;

    BabeSlotNumber genesis_slot_number_;
    BabeSlotNumber epoch_length_;

    // optimization for storing in memory last epoch
    boost::optional<EpochDescriptor> last_epoch_;
  };
}  // namespace kagome::consensus
#endif  // KAGOME_CONSENSUS_BABEUTILIMPL
