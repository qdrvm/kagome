/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/types.hpp"

namespace kagome::consensus::beefy {
  class FetchJustification {
   public:
    virtual ~FetchJustification() = default;
    virtual void fetchJustification(primitives::BlockNumber) = 0;
  };
}  // namespace kagome::consensus::beefy
