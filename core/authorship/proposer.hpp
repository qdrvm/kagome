/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_AUTHORSHIP_PROPOSER_TEST_HPP
#define KAGOME_CORE_AUTHORSHIP_PROPOSER_TEST_HPP

#include "clock/clock.hpp"
#include "primitives/block.hpp"
#include "primitives/block_id.hpp"
#include "primitives/digest.hpp"
#include "primitives/inherent_data.hpp"

namespace kagome::authorship {

  /**
   * Create block to further proposal for consensus
   */
  class Proposer {
   public:
    virtual ~Proposer() = default;

    /**
     * Creates block from provided parameters
     * @param parent_block_number number of parent
     * @param inherent_data additional data on block from unsigned extrinsics
     * @param inherent_digests - chain-specific block auxiliary data
     * @return proposed block or error
     */
    virtual outcome::result<primitives::Block> propose(
        const primitives::BlockNumber &parent_block_number,
        const primitives::InherentData &inherent_data,
        const primitives::Digest &inherent_digest) = 0;
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_PROPOSER_TEST_HPP
