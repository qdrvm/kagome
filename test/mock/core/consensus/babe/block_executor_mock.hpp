/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKEXECUTORMOCK
#define KAGOME_CONSENSUS_BLOCKEXECUTORMOCK

#include "consensus/babe/block_executor.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class BlockExecutorMock : public BlockExecutor {
   public:
    MOCK_METHOD(outcome::result<void>,
                applyBlock,
                (const primitives::BlockData &block),
                ());
    outcome::result<void> applyBlock(primitives::BlockData &&block) override {
      return applyBlock(block);
    }
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_BLOCKEXECUTORMOCK
