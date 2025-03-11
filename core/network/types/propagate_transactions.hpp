/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/transaction.hpp"

namespace kagome::network {
  /**
   * Propagate transactions in network
   */
  struct PropagatedExtrinsics {
    std::vector<primitives::Extrinsic> extrinsics;
  };
}  // namespace kagome::network
