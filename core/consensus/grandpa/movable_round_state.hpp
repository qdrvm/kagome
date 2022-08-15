/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_MOVABLEROUNDSTATE
#define KAGOME_CONSENSUS_GRANDPA_MOVABLEROUNDSTATE

#include <optional>

#include "consensus/grandpa/structs.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::grandpa {

  /// Stores the current state of the round
  struct MovableRoundState {
    SCALE_TIE(4);
    SCALE_TIE_EQ(MovableRoundState);

    RoundNumber round_number;
    BlockInfo last_finalized_block;
    std::vector<VoteVariant> votes;
    std::optional<BlockInfo> finalized;
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_MOVABLEROUNDSTATE
