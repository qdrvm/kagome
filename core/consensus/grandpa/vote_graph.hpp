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
      std::vector<BlockHash> descendents{};
      VoteWeight cumulative_vote;

      bool operator==(const Entry &o) const {
        // clang-format off
        return number == o.number
               && ancestors == o.ancestors
               && descendents == o.descendents
               && cumulative_vote == o.cumulative_vote;
        // clang-format on
      }

      // Get ancestor block by number. Returns `None` if there is no block
      // by that number in the direct ancestry.
      boost::optional<BlockHash> getAncestorBlockBy(BlockNumber n) const {
        if (n >= number) {
          return boost::none;
        }
        const auto offset = number - n - 1u;
        if (offset >= ancestors.size()) {
          return boost::none;
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

    virtual const BlockInfo &getBase() const = 0;

    /// Adjust the base of the graph. The new base must be an ancestor of the
    /// old base.
    ///
    /// Provide an ancestry proof from the old base to the new. The proof
    /// should be in reverse order from the old base's parent.
    virtual void adjustBase(const std::vector<BlockHash> &ancestry_proof) = 0;

    inline virtual outcome::result<void> insert(const Vote &vote,

                                                const VoteWeight &weigth) {
      return visit_in_place(
          vote, [this, &weigth](const auto &vote) -> outcome::result<void> {
            return insert(BlockInfo{vote.block_number, vote.block_hash},
                          weigth);
          });
    }

    inline virtual outcome::result<void> insert(const Prevote &prevote,
                                                const VoteWeight &vote) {
      return insert(BlockInfo{prevote.block_number, prevote.block_hash}, vote);
    }

    inline virtual outcome::result<void> insert(const Precommit &precommit,
                                                const VoteWeight &vote) {
      return insert(BlockInfo{precommit.block_number, precommit.block_hash},
                    vote);
    }

    /// Insert a vote with given value into the graph at given hash and number.
    virtual outcome::result<void> insert(const BlockInfo &block,
                                         const VoteWeight &vote) = 0;

    /// Find the highest block which is either an ancestor of or equal to the
    /// given, which fulfills a condition.
    virtual boost::optional<BlockInfo> findAncestor(
        const BlockInfo &block, const Condition &condition) const = 0;

    /// Find the best GHOST descendent of the given block.
    /// Pass a closure used to evaluate the cumulative vote value.
    ///
    /// The GHOST (hash, number) returned will be the block with highest number
    /// for which the cumulative votes of descendents and itself causes the
    /// closure to evaluate to true.
    ///
    /// This assumes that the evaluation closure is one which returns true for
    /// at most a single descendent of a block, in that only one fork of a block
    /// can be "heavy" enough to trigger the threshold.
    ///
    /// Returns `None` when the given `current_best` does not fulfill the
    /// condition.
    virtual boost::optional<BlockInfo> findGhost(
        const boost::optional<BlockInfo> &current_best,
        const Condition &condition) const = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_HPP
