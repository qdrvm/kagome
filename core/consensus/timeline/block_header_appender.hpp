/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKAPPENDER
#define KAGOME_CONSENSUS_BLOCKAPPENDER

#include "consensus/grandpa/environment.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"

namespace kagome::consensus {

  /**
   * Adds a new block header to the block storage
   */
  class BlockHeaderAppender {
   public:
    using ApplyJustificationCb = grandpa::Environment::ApplyJustificationCb;
    virtual ~BlockHeaderAppender() = default;

    virtual void appendHeader(
        primitives::BlockHeader &&block_header,
        const std::optional<primitives::Justification> &justification,
        ApplyJustificationCb &&callback) = 0;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_BLOCKAPPENDER
