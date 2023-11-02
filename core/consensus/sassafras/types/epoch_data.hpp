/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/sassafras/types/randomness.hpp"
#include "primitives/authority.hpp"

namespace kagome::consensus::sassafras {

  /// Data are corresponding to the epoch
  struct EpochData {
    SCALE_TIE(2);

    /// The authorities actual for corresponding epoch
    primitives::AuthorityIds authorities;

    /// The value of randomness to use for the slot-assignment.
    Randomness randomness;
  };

}  // namespace kagome::consensus::sassafras
