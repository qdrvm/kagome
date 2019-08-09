/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_VALIDATOR_HPP
#define KAGOME_BLOCK_VALIDATOR_HPP

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

    /**
     * Validate the block
     * @param block to be validated
     * @param peer, which has produced the block
     * @param epoch, in which the block arrived
     * @return nothing or validation error
     *
     * @note in case of success validation, the block is inserted into the local
     * blockchain state (tree or anything else)
     */
    virtual outcome::result<void> validate(const primitives::Block &block,
                                           const libp2p::peer::PeerId &peer,
                                           const Epoch &epoch) = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BLOCK_VALIDATOR_HPP
