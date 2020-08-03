/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA
#define KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA

#include "consensus/grandpa/round_observer.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Launches grandpa voting rounds
   */
  class Grandpa : public RoundObserver {
   public:
    ~Grandpa() override = default;
    /**
     * Start event loop which executes grandpa voting rounds
     */
    virtual void start() = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA
