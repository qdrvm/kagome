/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/types.hpp"

namespace kagome::consensus::beefy {
  /**
   * Fetch beefy justification.
   */
  class FetchJustification {
   public:
    virtual ~FetchJustification() = default;
    /**
     * Try fetch beefy justification for block.
     */
    virtual void fetchJustification(primitives::BlockNumber) = 0;
  };
}  // namespace kagome::consensus::beefy
