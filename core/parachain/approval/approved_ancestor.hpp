/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/common.hpp"

namespace kagome::parachain {
  /**
   * Provide parachain approval information to grandpa.
   */
  struct IApprovedAncestor {
    virtual ~IApprovedAncestor() = default;

    /**
     * Return highest fully approved block.
     * https://github.com/paritytech/polkadot/blob/3999688e2cd10dcb48db987b9550049160f9e25d/node/core/approval-voting/src/lib.rs#L1434
     */
    virtual primitives::BlockInfo approvedAncestor(
        const primitives::BlockInfo &min,
        const primitives::BlockInfo &max) const = 0;
  };
}  // namespace kagome::parachain
