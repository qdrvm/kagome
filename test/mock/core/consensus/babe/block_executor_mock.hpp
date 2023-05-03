/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKEXECUTORMOCK
#define KAGOME_CONSENSUS_BLOCKEXECUTORMOCK

#include "consensus/babe/block_executor.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

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

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BLOCKEXECUTORMOCK
