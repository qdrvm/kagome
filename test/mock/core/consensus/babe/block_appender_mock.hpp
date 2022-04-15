/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKAPPENDERMOCK
#define KAGOME_CONSENSUS_BLOCKAPPENDERMOCK

#include "consensus/babe/block_appender.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class BlockAppenderMock : public BlockAppender {
   public:
    MOCK_METHOD(outcome::result<void>,
                appendBlock,
                (const primitives::BlockData &block),
                ());
    outcome::result<void> appendBlock(primitives::BlockData &&block) override {
      return appendBlock(block);
    }
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_BLOCKAPPENDERMOCK
