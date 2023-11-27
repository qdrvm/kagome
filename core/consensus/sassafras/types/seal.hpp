/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/bandersnatch_types.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::sassafras {
  /**
   * Basically a signature of the block's header
   */
  struct Seal {
    SCALE_TIE(1);

    crypto::BandersnatchSignature signature;
  };
}  // namespace kagome::consensus::sassafras
