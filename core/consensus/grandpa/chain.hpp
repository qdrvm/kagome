/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_CHAIN_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_CHAIN_HPP

#include <vector>

#include <boost/optional.hpp>
#include <outcome/outcome.hpp>
#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /// Chain context necessary for implementation of the finality gadget.
  struct Chain {
    virtual ~Chain() = default;

    /**
     * @brief Get the ancestry of a {@param block} up to but not including the
     * {@param base} hash. Should be in reverse order from block's parent.
     * @return If the block is not a descendent of base, returns an error.
     */
    virtual outcome::result<std::vector<primitives::BlockHash>> getAncestry(
        primitives::BlockHash base, primitives::BlockHash block) = 0;

    /**
     * @returns the hash of the best block whose chain contains the given
     * block hash, even if that block is {@param base} itself. If base is
     * unknown, return None.
     */
    virtual boost::optional<BlockInfo> bestChainContaining(
        primitives::BlockHash base) = 0;

    /**
     * @returns true if {@param block} is a descendent of or equal to the
     * given {@param base}.
     */
    bool isEqualOrDescendOf(primitives::BlockHash base,
                            primitives::BlockHash block) {
      return base == block ? true : getAncestry(base, block).has_value();
    }
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_CHAIN_HPP
