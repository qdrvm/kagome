/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKEXECUTOR
#define KAGOME_CONSENSUS_BLOCKEXECUTOR

#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"

namespace kagome::consensus {

  class BlockExecutor {
   public:
    virtual ~BlockExecutor() = default;

    virtual outcome::result<void> applyBlock(primitives::BlockData &&block) = 0;

    virtual outcome::result<void> applyJustification(
        const primitives::BlockInfo &block_info,
        const primitives::Justification &justification) = 0;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_BLOCKEXECUTOR
