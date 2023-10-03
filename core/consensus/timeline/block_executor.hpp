/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/environment.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"

namespace kagome::consensus {

  class BlockExecutor {
   public:
    using ApplyJustificationCb = grandpa::Environment::ApplyJustificationCb;
    virtual ~BlockExecutor() = default;

    virtual void applyBlock(
        primitives::Block &&block,
        const std::optional<primitives::Justification> &justification,
        ApplyJustificationCb &&callback) = 0;
  };

}  // namespace kagome::consensus
