/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_LAUNCHER_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_LAUNCHER_HPP

#include "consensus/grandpa/round_observer.hpp"

namespace kagome::consensus::grandpa {

  class Launcher : public RoundObserver {
   public:
    ~Launcher() override = default;
    virtual void start() = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_LAUNCHER_HPP
