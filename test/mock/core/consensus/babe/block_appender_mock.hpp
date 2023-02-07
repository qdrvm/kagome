/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKAPPENDERMOCK
#define KAGOME_CONSENSUS_BLOCKAPPENDERMOCK

#include "consensus/babe/block_header_appender.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

  class BlockAppenderMock : public BlockHeaderAppender {
   public:
    MOCK_METHOD(outcome::result<void>,
                appendBlock,
                (const primitives::BlockData &block),
                ());
    outcome::result<void> appendBlock(primitives::BlockData &&block) override {
      return appendBlock(block);
    }

    MOCK_METHOD(outcome::result<void>,
                applyJustification,
                (const primitives::BlockInfo &block_info,
                 const primitives::Justification &justification),
                (override));
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BLOCKAPPENDERMOCK
