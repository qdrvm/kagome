/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MOCK_PARACHAIN_APPROVAL_APPROVED_ANCESTOR_HPP
#define KAGOME_MOCK_PARACHAIN_APPROVAL_APPROVED_ANCESTOR_HPP

#include "parachain/approval/approved_ancestor.hpp"

namespace kagome::parachain {
  struct ApprovedAncestorMock : IApprovedAncestor {
    MOCK_METHOD(primitives::BlockInfo,
                approvedAncestor,
                (const primitives::BlockInfo &, const primitives::BlockInfo &),
                (const, override));
  };
}  // namespace kagome::parachain

#endif  // KAGOME_MOCK_PARACHAIN_APPROVAL_APPROVED_ANCESTOR_HPP
