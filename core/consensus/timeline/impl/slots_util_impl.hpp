/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/slots_util.hpp"

#include "log/logger.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::application {
  class AppStateManager;
}

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::consensus {
  class ConsensusSelector;
}

namespace kagome::storage {
  class SpacedStorage;
}

namespace kagome::consensus {

  class SlotsUtilImpl final : public SlotsUtil {
   public:
    SlotsUtilImpl(application::AppStateManager &app_state_manager,
                  std::shared_ptr<storage::SpacedStorage> persistent_storage,
                  std::shared_ptr<blockchain::BlockTree> block_tree,
                  std::shared_ptr<ConsensusSelector> consensus_selector);

    bool prepare();

    Duration slotDuration() const override;

    EpochLength epochLength() const override;

    SlotNumber timeToSlot(TimePoint time) const override;

    TimePoint slotStartTime(SlotNumber slot) const override;
    TimePoint slotFinishTime(SlotNumber slot) const override;

    outcome::result<EpochDescriptor> slotToEpochDescriptor(
        const primitives::BlockInfo &parent_info,
        SlotNumber slot) const override;

   private:
    outcome::result<SlotNumber> getFirstBlockSlotNumber(
        const primitives::BlockInfo &parent_info) const;

    log::Logger log_;
    std::shared_ptr<storage::BufferStorage> persistent_storage_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<ConsensusSelector> consensus_selector_;

    Duration slot_duration_{};
    EpochLength epoch_length_{};

    mutable std::optional<SlotNumber> first_block_slot_number_;
  };

}  // namespace kagome::consensus
