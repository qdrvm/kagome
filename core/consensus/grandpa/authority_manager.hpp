/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/optional.hpp>

#include "common/tagged.hpp"
#include "consensus/grandpa/types/authority.hpp"
#include "primitives/block_data.hpp"
#include "primitives/block_header.hpp"

namespace kagome::storage::trie {
  class TrieStorage;
  class TrieBatch;
}  // namespace kagome::storage::trie

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::consensus::grandpa {

  using IsBlockFinalized = Tagged<bool, struct IsBlockFinalizedTag>;

  class AuthorityManager {
   public:
    virtual ~AuthorityManager() = default;

    /**
     * @brief Returns authorities according specified block
     * @param block for which authority set is requested
     * @param finalized - true if we consider that the provided block is
     * finalized
     * @return outcome authority set
     */
    virtual std::optional<std::shared_ptr<const AuthoritySet>> authorities(
        const primitives::BlockInfo &block,
        IsBlockFinalized finalized) const = 0;

    /// Find previous scheduled change with justification
    using ScheduledParentResult =
        outcome::result<std::pair<primitives::BlockInfo, AuthoritySetId>>;
    virtual ScheduledParentResult scheduledParent(
        primitives::BlockInfo block) const = 0;

    /// Find possible scheduled changes with justification
    virtual std::vector<primitives::BlockInfo> possibleScheduled() const = 0;

    /**
     * Warp synced to `block` with `authorities`.
     */
    virtual void warp(const primitives::BlockInfo &block,
                      const primitives::BlockHeader &header,
                      const AuthoritySet &authorities) = 0;
  };
}  // namespace kagome::consensus::grandpa
