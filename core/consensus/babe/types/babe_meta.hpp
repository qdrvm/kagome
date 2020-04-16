/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
