/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_VOTE_GRAPH_IMPL_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_VOTE_GRAPH_IMPL_HPP

#include "consensus/grandpa/vote_graph.hpp"

#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace kagome::consensus::grandpa {

  class VoteGraphImpl : public VoteGraph {
   public:
    VoteGraphImpl(const BlockInfo &base,
                  std::shared_ptr<VoterSet> voter_set,
                  std::shared_ptr<Chain> chain);

    /// Adjust the base of the graph. The new base must be an ancestor of the
    /// old base.
    ///
    /// Provide an ancestry proof from the old base to the new. The proof
    /// should be in reverse order from the old base's parent.
    void adjustBase(const std::vector<BlockHash> &ancestry_proof) override;

    /// Insert vote {@param vote_type} of {@param voter} for {@param block}
    outcome::result<void> insert(VoteType vote_type,
                                 const BlockInfo &block,
                                 const Id &voter) override;

    /// Remove vote {@param vote_type} of {@param voter}
    void remove(VoteType vote_type, const Id &voter) override;

    /// Find the highest block which is either an ancestor of or equal to the
    /// given, which fulfills a condition.
    std::optional<BlockInfo> findAncestor(
        VoteType vote_type,
        const BlockInfo &block,
        const Condition &condition) const override;

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
    std::optional<BlockInfo> findGhost(
        VoteType vote_type,
        const std::optional<BlockInfo> &current_best,
        const Condition &condition) const override;

    // introduce a branch to given vote-nodes.
    //
    // `descendants` is a list of nodes with ancestor-edges containing the given
    // ancestor.
    //
    // This function panics if any member of `descendants` is not a vote-node
    // or does not have ancestor with given hash and number OR if
    // `ancestor_hash` is already a known entry.
    void introduceBranch(const std::vector<primitives::BlockHash> &descendants,
                         const BlockInfo &ancestor);

    // append a vote-node onto the chain-tree. This should only be called if
    // no node in the tree keeps the target anyway.
    outcome::result<void> append(const BlockInfo &block);

    // given a key, node pair (which must correspond), assuming this node
    // fulfills the condition, this function will find the highest point at
    // which its descendants merge, which may be the node itself.
    Subchain ghostFindMergePoint(
        VoteType vote_type,
        const BlockHash &active_node_hash,
        const Entry &active_node,
        const std::optional<BlockInfo> &force_constrain,
        const Condition &condition) const;

    // attempts to find the containing node keys for the given hash and number.
    //
    // returns `None` if there is a node by that key already, and a vector
    // (potentially empty) of nodes with the given block in its ancestor-edge
    // otherwise.
    std::optional<std::vector<primitives::BlockHash>> findContainingNodes(
        const BlockInfo &block) const;

    const BlockInfo &getBase() const override {
      return base_;
    }

    // should be mutable, otherwise operator[] is not defined for const map
    const auto &getEntries() const {
      return entries_;
    }

    const auto &getHeads() const {
      return heads_;
    }

   private:
    BlockInfo base_;
    std::shared_ptr<VoterSet> voter_set_;
    std::shared_ptr<Chain> chain_;

    std::unordered_map<BlockHash, Entry> entries_;
    std::unordered_set<BlockHash> heads_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_VOTE_GRAPH_IMPL_HPP
