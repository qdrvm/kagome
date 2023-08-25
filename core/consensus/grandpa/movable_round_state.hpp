/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "consensus/grandpa/structs.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::grandpa {

  /// Stores the current state of the round
  struct MovableRoundState {
    SCALE_TIE(4);

    RoundNumber round_number;
    BlockInfo last_finalized_block;
    std::vector<VoteVariant> votes;
    std::optional<BlockInfo> finalized;
  };
}  // namespace kagome::consensus::grandpa
