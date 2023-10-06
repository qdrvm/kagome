/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/approval/approved_ancestor.hpp"

namespace kagome::parachain {
  struct ApprovedAncestorMock : IApprovedAncestor {
    MOCK_METHOD(primitives::BlockInfo,
                approvedAncestor,
                (const primitives::BlockInfo &, const primitives::BlockInfo &),
                (const, override));
  };
}  // namespace kagome::parachain
