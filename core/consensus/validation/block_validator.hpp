/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_VALIDATOR_HPP
#define KAGOME_BLOCK_VALIDATOR_HPP

#include <memory>

#include <gsl/span>
#include <outcome/outcome.hpp>
#include "consensus/babe/common.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "primitives/block.hpp"

namespace kagome::consensus {
  /**
   * Validator of the blocks
   */
  struct BlockValidator {
    virtual ~BlockValidator() = default;

    // TODO(akvinikym): last three parameters should be in BabeEpoch struct
    /**
     * Validate the block
     * @param block to be validated
     * @param peer, which has produced the block
     * @param authorities of this epoch
     * @param randomness of this epoch
     * @param threshold of this epoch
     * @return nothing or validation error
     */
    virtual outcome::result<void> validate(
        const primitives::Block &block,
        const libp2p::peer::PeerId &peer,
        gsl::span<const Authority> authorities,
        const Randomness &randomness,
        const Threshold &threshold) = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BLOCK_VALIDATOR_HPP
