/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/catch_up_observer.hpp"
#include "consensus/grandpa/justification_observer.hpp"
#include "consensus/grandpa/neighbor_observer.hpp"
#include "consensus/grandpa/round_observer.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Launches grandpa voting rounds
   */
  class GrandpaObserver : public RoundObserver,
                          public JustificationObserver,
                          public CatchUpObserver,
                          public NeighborObserver {
   public:
    ~GrandpaObserver() override = default;
  };

}  // namespace kagome::consensus::grandpa
