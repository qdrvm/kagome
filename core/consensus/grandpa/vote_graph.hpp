/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_HPP

#include <boost/operators.hpp>

#include "consensus/grandpa/chain.hpp"
#include "consensus/grandpa/vote_weight.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Keeps track of obtained votes. Provides convenient interfaces for getting
   * ancestry of the votes and calculating ghost vote
   */
  struct VoteGraph {
    // graph entry
    struct Entry : public boost::equality_comparable<Entry> {
      BlockNumber number{};
      std::vector<BlockHash> ancestors{};
      std::vector<BlockHash> descendants{};
      VoteWeight cumulative_vote;

      bool operator==(const Entry &o) const {
        // clang-format off
        return number == o.number
               && ancestors == o.ancestors
               && descendants == o.descendants
               && cumulative_vote == o.cumulative_vote;
        // clang-format on
      }

      // Get ancestor block by number. Returns `None` if there is no block
      // by that number in the direct ancestry.
      std::optional<BlockHash> getAncestorBlockBy(BlockNumber n) const {
        if (n >= number) {
          return std::nullopt;
        }
        const auto offset = number - n - 1u;
        if (offset >= ancestors.size()) {
          return std::nullopt;
        }
        return ancestors[offset];
      }
    };

    struct Subchain {
      std::vector<BlockHash> hashes;
      BlockNumber best_number;
    };

    virtual ~VoteGraph() = default;

    using Condition = std::function<bool(const VoteWeight &)>;
    using Comparator =
        std::function<bool(const VoteWeight &, const VoteWeight &)>;

    virtual const BlockInfo &getBase() const = 0;

    /// Adjust the base of the graph. The new base must be an ancestor of the
    /// old base.
    ///
    /// Provide an ancestry proof from the old base to the new. The proof
    /// should be in reverse order from the old base's parent.
    virtual void adjustBase(const std::vector<BlockHash> &ancestry_proof) = 0;

    /// Insert a {@param vote} of {@param voter}
    virtual outcome::result<void> insert(const BlockInfo &vote, Id voter) = 0;

    /// Remove a {@param vote} of {@param voter}
    virtual void remove(Id voter) = 0;

    /// Find the highest block which is either an ancestor of or equal to the
    /// given, which fulfills a condition.
    virtual std::optional<BlockInfo> findAncestor(
        const BlockInfo &block, const Condition &condition) const = 0;

    /// Find the best GHOST descendant of the given block.
    /// Pass a closure used to evaluate the cumulative vote value.
    ///
    /// The GHOST (hash, number) returned will be the block with highest number
    /// for which the cumulative votes of descendants and itself causes the
    /// closure to evaluate to true.
    ///
    /// This assumes that the evaluation closure is one which returns true for
    /// at most a single descendant of a block, in that only one fork of a block
    /// can be "heavy" enough to trigger the threshold.
    ///
    /// Returns `None` when the given `current_best` does not fulfill the
    /// condition.
    virtual std::optional<BlockInfo> findGhost(
        const std::optional<BlockInfo> &current_best,
        const Condition &condition) const = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_HPP
