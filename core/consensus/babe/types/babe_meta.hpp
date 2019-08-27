/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_META_HPP
#define KAGOME_BABE_META_HPP

#include "clock/clock.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "consensus/babe/common.hpp"
#include "consensus/babe/types/epoch.hpp"

namespace kagome::consensus {
  /**
   * Information about running BABE production
   */
  struct BabeMeta {
    const Epoch &current_epoch_;
    BabeSlotNumber current_slot_{};
    const BabeLottery::SlotsLeadership &slots_leadership_;
    const BabeTimePoint &last_slot_finish_time_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_META_HPP
