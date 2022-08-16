/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_EPOCHDIGEST
#define KAGOME_CONSENSUS_EPOCHDIGEST

#include "consensus/babe/common.hpp"
#include "primitives/authority.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus {

  /// Data are corresponding to the epoch
  struct EpochDigest {
    SCALE_TIE(2);
    SCALE_TIE_EQ(EpochDigest);

    /// The authorities actual for corresponding epoch
    primitives::AuthorityList authorities;

    /// The value of randomness to use for the slot-assignment.
    Randomness randomness;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_EPOCHDIGEST
