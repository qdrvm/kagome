/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_GRANDPA
#define KAGOME_CONSENSUS_GRANDPA_GRANDPA

#include "consensus/grandpa/common.hpp"

#include <memory>

namespace kagome::consensus::grandpa {
  class VotingRound;
}

namespace kagome::consensus::grandpa {

  /**
   * Interface for launching new grandpa rounds. See more details in
   * kagome::consensus::grandpa::GrandpaImpl
   */
  class Grandpa {
   public:
    virtual ~Grandpa() = default;

    /**
     * Tries to execute the next round
     * Round may not be executed if prev round is not equal to our current round
     *
     * @param prev_round - round for which tries to execute the next one
     */
    virtual void tryExecuteNextRound(
        const std::shared_ptr<VotingRound> &prev_round) = 0;

    /**
     * Force update for the round. Next round to the provided one will be
     * checked and updated to the new prevote ghost (if any), round estimate (if
     * any), finalized block (if any) and completability
     * @param round_number the round number the following to which is updated
     */
    virtual void updateNextRound(RoundNumber round_number) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_GRANDPA
