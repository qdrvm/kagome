/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA
#define KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA

#include "consensus/grandpa/grandpa_observer.hpp"
#include "consensus/grandpa/voting_round.hpp"

namespace kagome::consensus::grandpa {

  class VotingRound;

  /**
   * Launches grandpa voting rounds
   */
  class Grandpa {
   public:
    virtual ~Grandpa() = default;

    /**
     * Tries to execute next round for presented one
     * @param round - round for which tries to execute the next
     */
    virtual void executeNextRound(
        const std::shared_ptr<VotingRound> &prev_round) = 0;

    /**
     * Force update round for round presented by {@param round_number}
     */
    virtual void updateNextRound(RoundNumber round_number) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA
