/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA
#define KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA

#include "consensus/grandpa/grandpa_observer.hpp"

namespace kagome::consensus::grandpa {

  class VotingRound;

  /**
   * Launches grandpa voting rounds
   */
  class Grandpa {
   public:
    virtual ~Grandpa() = default;

    virtual void executeNextRound(
        const std::shared_ptr<VotingRound> &round) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA
