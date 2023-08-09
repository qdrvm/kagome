/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_APPROVAL_APPROVED_ANCESTOR_HPP
#define KAGOME_PARACHAIN_APPROVAL_APPROVED_ANCESTOR_HPP

#include "primitives/common.hpp"

namespace kagome::parachain {
  struct IApprovedAncestor {
    virtual ~IApprovedAncestor() = default;

    virtual primitives::BlockInfo approvedAncestor(
        const primitives::BlockInfo &min, const primitives::BlockInfo &max) = 0;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_APPROVAL_APPROVED_ANCESTOR_HPP
