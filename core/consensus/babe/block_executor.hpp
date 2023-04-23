/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKEXECUTOR
#define KAGOME_CONSENSUS_BLOCKEXECUTOR

#include "consensus/grandpa/environment.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"

namespace kagome::consensus::babe {

  class BlockExecutor {
   public:
    using ApplyJustificationCb = grandpa::Environment::ApplyJustificationCb;
    virtual ~BlockExecutor() = default;

    virtual void applyBlock(
        primitives::Block &&block,
        std::optional<primitives::Justification> const &justification,
        ApplyJustificationCb &&callback) = 0;
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BLOCKEXECUTOR
