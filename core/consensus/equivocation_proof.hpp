/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/types.hpp"
#include "primitives/block_header.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus {

  /// Represents an equivocation proof. An equivocation happens when a validator
  /// produces more than one block on the same slot. The proof of equivocation
  /// are the given distinct headers that were signed by the validator and which
  /// include the slot number.
  struct EquivocationProof {
    SCALE_TIE(4);

    /// Returns the authority id of the equivocator.
    primitives::AuthorityId offender;
    /// The slot at which the equivocation happened.
    SlotNumber slot;
    /// The first header involved in the equivocation.
    primitives::BlockHeader first_header;
    /// The second header involved in the equivocation.
    primitives::BlockHeader second_header;
  };

}  // namespace kagome::consensus
