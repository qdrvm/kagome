/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_TYPES_SLOTS_STRATEGY_HPP
#define KAGOME_CORE_CONSENSUS_BABE_TYPES_SLOTS_STRATEGY_HPP

namespace kagome::consensus {

  /**
   * Strategy defining how calculate slots number
   */
  enum class SlotsStrategy {
    FromZero,      // First slot will have number 0
    FromUnixEpoch  // First slot will have number `time_from_unix_epoch() /
                   // slot_duration`
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_BABE_TYPES_SLOTS_STRATEGY_HPP
