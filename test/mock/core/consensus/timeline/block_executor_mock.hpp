/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/block_executor.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class BlockExecutorMock : public BlockExecutor {
   public:
    MOCK_METHOD(void,
                applyBlock,
                (const primitives::Block &block,
                 const std::optional<primitives::Justification> &justification,
                 ApplyJustificationCb &&callback),
                ());

    void applyBlock(
        primitives::Block &&block,
        const std::optional<primitives::Justification> &justification,
        ApplyJustificationCb &&callback) override {
      return applyBlock(block, justification, std::move(callback));
    }
  };

}  // namespace kagome::consensus
