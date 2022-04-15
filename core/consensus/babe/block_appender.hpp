/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKAPPENDER
#define KAGOME_CONSENSUS_BLOCKAPPENDER

#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"

namespace kagome::consensus {

  class BlockAppender {
   public:
    virtual ~BlockAppender() = default;

    virtual outcome::result<void> appendBlock(primitives::BlockData &&b) = 0;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_BLOCKAPPENDER
