/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_AUTHORSHIP_PROPOSER_TEST_HPP
#define KAGOME_CORE_AUTHORSHIP_PROPOSER_TEST_HPP

#include "clock/clock.hpp"
#include "primitives/block.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"
#include "primitives/inherent_data.hpp"
#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::authorship {

  /**
   * Create block to further proposal for consensus
   */
  class Proposer {
   public:
    using Clock = clock::SystemClock;

    virtual ~Proposer() = default;

    /**
     * Creates block from provided parameters
     * @param parent_block number and hash of parent block
     * @param inherent_data additional data on block from unsigned extrinsics
     * @param inherent_digests - chain-specific block auxiliary data
     * @return proposed block or error
     */
    virtual outcome::result<primitives::Block> propose(
        const primitives::BlockInfo &parent_block,
        std::optional<Clock::TimePoint> deadline,
        const primitives::InherentData &inherent_data,
        const primitives::Digest &inherent_digest,
        TrieChangesTrackerOpt changes_tracker) = 0;
  };

}  // namespace kagome::authorship

#endif  // KAGOME_CORE_AUTHORSHIP_PROPOSER_TEST_HPP
