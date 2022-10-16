/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_VALIDATOR_HPP
#define KAGOME_BLOCK_VALIDATOR_HPP

#include <outcome/outcome.hpp>
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
     * @param randomness is randomness used in this epoch
     * @return nothing or validation error
     */
    virtual outcome::result<void> validateHeader(
        const primitives::BlockHeader &block_header,
        const EpochNumber epoch_number,
        const primitives::AuthorityId &authority_id,
        const Threshold &threshold,
        const primitives::BabeConfiguration& babe_config) const = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BLOCK_VALIDATOR_HPP
