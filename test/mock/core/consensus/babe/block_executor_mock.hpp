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
    MOCK_METHOD(outcome::result<void>,
                applyBlock,
                (const primitives::Block &block,
                 std::optional<primitives::Justification> const &justification),
                ());

    outcome::result<void> applyBlock(
        primitives::Block &&block,
        std::optional<primitives::Justification> const &justification)
        override {
      return applyBlock(block, justification);
    }
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BLOCKEXECUTORMOCK
