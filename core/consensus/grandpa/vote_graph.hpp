/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_HPP

#include "consensus/grandpa/chain.hpp"
#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  struct VoteGraph {
    // graph entry
    struct Entry {
      primitives::BlockNumber number;
      std::vector<primitives::BlockHash> ancestors;
      std::vector<primitives::BlockHash> descendents;
      // cumulative vote?
    };

    struct Subchain {
      std::vector<primitives::BlockHash> hashes;
      primitives::BlockNumber best_number;
    };

    virtual ~VoteGraph() = default;

    using Condition = std::function<bool(/*vote weight*/)>;

    explicit VoteGraph(const BlockInfo &base) : base_(base) {}

    const auto &getBase() const {
      return base_;
    }

    /// Adjust the base of the graph. The new base must be an ancestor of the
    /// old base.
    ///
    /// Provide an ancestry proof from the old base to the new. The proof
    /// should be in reverse order from the old base's parent.
    virtual void adjustBase(
        std::vector<primitives::BlockHash> ancestry_proof) = 0;

    /// Insert a vote with given value into the graph at given hash and number.
    virtual outcome::result<void> insert(
        const BlockInfo &block,
        /*vote weight */ std::shared_ptr<Chain> chain) = 0;

    /// Find the highest block which is either an ancestor of or equal to the
    /// given, which fulfills a condition.
    virtual boost::optional<BlockInfo> findAncestor(const BlockInfo &block,
                                                    Condition cond) = 0;

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
    virtual boost::optional<BlockInfo> findGhost(const BlockInfo &current_best,
                                                 Condition cond) = 0;

    // given a key, node pair (which must correspond), assuming this node
    // fulfills the condition, this function will find the highest point at
    // which its descendents merge, which may be the node itself.
    virtual Subchain ghostFindMergePoint(
        const primitives::BlockHash &nodeKey,
        Entry &activeNode,
        /* force constrain?*/ Condition cond) = 0;

    // attempts to find the containing node keys for the given hash and number.
    //
    // returns `None` if there is a node by that key already, and a vector
    // (potentially empty) of nodes with the given block in its ancestor-edge
    // otherwise.
    virtual boost::optional<std::vector<primitives::BlockHash>>
    findContainingNodes(const BlockInfo &block) = 0;

    // introduce a branch to given vote-nodes.
    //
    // `descendents` is a list of nodes with ancestor-edges containing the given
    // ancestor.
    //
    // This function panics if any member of `descendents` is not a vote-node
    // or does not have ancestor with given hash and number OR if
    // `ancestor_hash` is already a known entry.
    virtual void introduceBranch(
        const std::vector<primitives::BlockHash> &descendents,
        primitives::BlockHash ancestor,
        primitives::BlockNumber ancestor_number) = 0;

    // append a vote-node onto the chain-tree. This should only be called if
    // no node in the tree keeps the target anyway.
    virtual outcome::result<void> append(const BlockInfo &block,
                                         std::shared_ptr<Chain> chain) = 0;

   private:
    BlockInfo base_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_HPP
