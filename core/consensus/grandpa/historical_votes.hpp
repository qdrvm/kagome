/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {
  /**
   * Historical votes seen in a round.
   * https://github.com/paritytech/finality-grandpa/blob/8c45a664c05657f0c71057158d3ba555ba7d20de/src/lib.rs#L544
   */
  struct HistoricalVotes {
    SCALE_TIE(3);

    std::vector<SignedMessage> seen;
    std::optional<uint64_t> prevote_idx, precommit_idx;
  };

  class SaveHistoricalVotes {
   public:
    virtual ~SaveHistoricalVotes() = default;

    /**
     * Called from `VotingRoundImpl` to `GrandpaImpl` to save historical vote.
     */
    virtual void saveHistoricalVote(AuthoritySetId set,
                                    RoundNumber round,
                                    const SignedMessage &vote,
                                    bool set_index) = 0;
  };
}  // namespace kagome::consensus::grandpa
