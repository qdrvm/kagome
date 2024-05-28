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

    /**
     * Set the number of messages seen before prevoting.
     */
    void set_prevoted_index() {
      prevote_idx = seen.size();
    }

    /**
     * Set the number of messages seen before precommiting.
     */
    void set_precommitted_index() {
      precommit_idx = seen.size();
    }
  };

  class SaveHistoricalVotes {
   public:
    virtual ~SaveHistoricalVotes() = default;

    virtual void saveHistoricalVote(AuthoritySetId set,
                                    RoundNumber round,
                                    const SignedMessage &vote,
                                    bool set_index) = 0;
  };
}  // namespace kagome::consensus::grandpa
