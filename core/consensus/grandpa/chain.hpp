/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_CHAIN
#define KAGOME_CONSENSUS_GRANDPA_CHAIN

#include <optional>
#include <vector>

#include <outcome/outcome.hpp>

#include "consensus/grandpa/common.hpp"
#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {
  class VoterSet;
}

namespace kagome::consensus::grandpa {

  /**
   * Chain context necessary for implementation of the finality gadget.
   */
  struct Chain {
    virtual ~Chain() = default;

    /**
     * @brief Checks if {@param block} exists locally
     * @return true iff block exists
     */
    virtual outcome::result<bool> hasBlock(
        const primitives::BlockHash &block) const = 0;

    /**
     * @brief Get the ancestry of a {@param block} up to the {@param base} hash.
     * Should be in reverse order from block's parent.
     * @return If the block is not a descendant of base, returns an error.
     */
    virtual outcome::result<std::vector<primitives::BlockHash>> getAncestry(
        const primitives::BlockHash &base,
        const primitives::BlockHash &block) const = 0;

    /**
     * @brief Check if block is ancestor for second one
     * @param base is potential ancestor
     * @param block is testee block
     * @return true, if \param base is ancestor for \param block
     */
    virtual bool hasAncestry(const primitives::BlockHash &base,
                             const primitives::BlockHash &block) const = 0;

    /**
     * @returns the hash of the best block whose chain contains the given
     * block hash, even if that block is {@param base} itself. If base is
     * unknown, return None.
     */
    virtual outcome::result<primitives::BlockInfo> bestChainContaining(
        const primitives::BlockHash &base,
        std::optional<VoterSetId> voter_set_id) const = 0;

    /**
     * @returns true if {@param block} is a descendant of or equal to the
     * given {@param base}.
     */
    inline bool isEqualOrDescendOf(const primitives::BlockHash &base,
                                   const primitives::BlockHash &block) const {
      return base == block ? true : hasAncestry(base, block);
    }
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_CHAIN
