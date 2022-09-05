/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_EPOCHDESCRIPTOR
#define KAGOME_CONSENSUS_EPOCHDESCRIPTOR

#include "consensus/babe/common.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus {

  /**
   * Metainformation about the Babe epoch
   */
  struct EpochDescriptor {
    SCALE_TIE(2);

    EpochNumber epoch_number = 0;

    /// starting slot of the epoch
    BabeSlotNumber start_slot = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_EPOCHDESCRIPTOR
