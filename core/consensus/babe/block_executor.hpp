/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKEXECUTOR
#define KAGOME_CONSENSUS_BLOCKEXECUTOR

#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"

namespace kagome::consensus::babe {

  class BlockExecutor {
   public:
    virtual ~BlockExecutor() = default;

    virtual outcome::result<void> applyBlock(
        primitives::Block &&block,
        std::optional<primitives::Justification> const &justification) = 0;
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BLOCKEXECUTOR
