/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/babe/types/babe_configuration.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block.hpp"

namespace kagome::consensus {
  /**
   * Validator of the blocks
   */
  class BlockValidator {
   public:
    virtual ~BlockValidator() = default;

    /**
     * Validate the block header
     * @param block to be validated
     * @param authority_id authority that sent this block
     * @param threshold is vrf threshold for this epoch
     * @param config is babe config for this epoch
     * @return nothing or validation error
     */
    virtual outcome::result<void> validateHeader(
        const primitives::BlockHeader &block_header,
        const EpochNumber epoch_number,
        const primitives::AuthorityId &authority_id,
        const Threshold &threshold,
        const babe::BabeConfiguration &config) const = 0;
  };
}  // namespace kagome::consensus
