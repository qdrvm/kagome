/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/common.hpp"

namespace kagome::consensus {

  class FinalityConsensus {
   public:
    FinalityConsensus() = default;
    virtual ~FinalityConsensus() = default;

    FinalityConsensus(FinalityConsensus &&) = delete;
    FinalityConsensus(const FinalityConsensus &) = delete;
    FinalityConsensus &operator=(FinalityConsensus &&) = delete;
    FinalityConsensus &operator=(const FinalityConsensus &) = delete;
  };

}  // namespace kagome::consensus
