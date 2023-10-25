/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "scale/tie.hpp"

namespace kagome::consensus::sassafras::bandersnatch {

  /// Context used to produce ring signatures.
  class RingContext {
   public:
    SCALE_TIE(0);
  };

}  // namespace kagome::consensus::sassafras::bandersnatch
