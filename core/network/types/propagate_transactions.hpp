/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROPAGATED_TRANSACTIONS_HPP
#define KAGOME_NETWORK_PROPAGATED_TRANSACTIONS_HPP

#include "primitives/transaction.hpp"
#include "scale/tie.hpp"

namespace kagome::network {
  /**
   * Propagate transactions in network
   */
  struct PropagatedExtrinsics {
    SCALE_TIE(1);

    std::vector<primitives::Extrinsic> extrinsics;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROPAGATED_TRANSACTIONS_HPP
