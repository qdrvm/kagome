/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/structs.hpp"

namespace kagome::network {
  struct WarpSyncFragment {
    SCALE_TIE(2);

    primitives::BlockHeader header;
    consensus::grandpa::GrandpaJustification justification;
  };

  struct WarpSyncProof {
    SCALE_TIE(2);

    std::vector<WarpSyncFragment> proofs;
    bool is_finished = false;
  };
}  // namespace kagome::network
