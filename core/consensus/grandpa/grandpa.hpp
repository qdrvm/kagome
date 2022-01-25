/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA
#define KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA

#include "consensus/grandpa/grandpa_observer.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Launches grandpa voting rounds
   */
  class Grandpa {
   public:
    virtual ~Grandpa() = default;

    /**
     * Tries to execute next round for round presented by {@param round_number}
     */
    virtual void executeNextRound(RoundNumber round_number) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA
