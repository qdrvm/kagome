/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKAPPENDER
#define KAGOME_CONSENSUS_BLOCKAPPENDER

#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"

namespace kagome::consensus::babe {

  /**
   * Adds a new block header to the block storage
   */
  class BlockHeaderAppender {
   public:
    virtual ~BlockHeaderAppender() = default;

    virtual outcome::result<void> appendHeader(
        primitives::BlockHeader &&block_header,
        std::optional<primitives::Justification> const &justification) = 0;
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BLOCKAPPENDER
