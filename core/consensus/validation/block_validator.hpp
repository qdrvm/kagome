/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_VALIDATOR_HPP
#define KAGOME_BLOCK_VALIDATOR_HPP

#include <outcome/outcome.hpp>
#include "consensus/babe/types/epoch.hpp"
#include "primitives/block.hpp"

namespace kagome::consensus {
  /**
   * Validator of the blocks
   */
  struct BlockValidator {
    virtual ~BlockValidator() = default;

    /**
     * Validate the block
     * @param block to be validated
     * @param epoch, in which the block arrived
     * @return nothing or validation error
     *
     * @note in case of success validation, the block is inserted into the local
     * blockchain state (tree or anything else)
     */
    virtual outcome::result<void> validate(const primitives::Block &block,
                                           const Epoch &epoch) const = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BLOCK_VALIDATOR_HPP
