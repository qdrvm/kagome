/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_error.hpp"

#include <stack>

namespace kagome::consensus::grandpa {

  namespace {
    /// filters vector in-place - effectively removes all elements for which
    /// func returned false.
    template <typename Vector, typename Func>
    inline void filter_if(Vector &&v, Func &&func) {
      v.erase(std::remove_if(
                  v.begin(), v.end(), std::not_fn(std::forward<Func>(func))),
              v.end());
    }

    /// check if collection contains a given item.
    template <typename Collection, typename Item>
    inline bool contains(const Collection &collection, const Item &item) {
      auto it = collection.find(item);
      return it != collection.end();
    }

    // whether the given hash, number pair is a direct ancestor of this node.
    inline bool inDirectAncestry(const VoteGraph::Entry &entry,
                                 const BlockHash &hash,
                                 const BlockNumber &number) {
      // in direct ancestry?
      auto opt = entry.getAncestorBlockBy(number);
      return opt && opt == hash;
    }
  }  // namespace

  VoteGraphImpl::VoteGraphImpl(const BlockInfo &base,
                               std::shared_ptr<VoterSet> voter_set,
                               std::shared_ptr<Chain> chain)
      : base_(base),
        voter_set_(std::move(voter_set)),
        chain_(std::move(chain)) {
    entries_[base.hash].number = base.number;
    heads_.insert(base.hash);
  }

  std::optional<std::vector<primitives::BlockHash>>
  VoteGraphImpl::findContainingNodes(const BlockInfo &block) const {
    if (contains(entries_, block.hash)) {
      return std::nullopt;
    }

    std::vector<BlockHash> containing;
    std::unordered_set<BlockHash> visited;

    // iterate vote-heads and their ancestry backwards until we find the one
    // with this target hash in that chain.
    for (auto head : heads_) {
      for (auto it = entries_.find(head); it != entries_.end();
           it = entries_.find(head)) {
        const auto &active_entry = it->second;

        // if node has been checked already, break
        if (auto [_, inserted] = visited.emplace(head); not inserted) {
          break;
        }

        auto ancestor_opt = active_entry.getAncestorBlockBy(block.number);
        if (ancestor_opt.has_value()) {
          if (*ancestor_opt == block.hash) {
            containing.push_back(head);
          }
        } else {
          if (not active_entry.ancestors.empty()) {
            head = active_entry.ancestors.back();
            continue;
          }
        }

        break;
      }
    }

    return containing;
  }

  outcome::result<void> VoteGraphImpl::insert(VoteType vote_type,
                                              const BlockInfo &block,
                                              const Id &voter) {
    auto inw_res = voter_set_->indexAndWeight(voter);
    if (inw_res.has_error()) {
      return inw_res.as_failure();
    }
    const auto [index, weight] = inw_res.value();

    if (auto containing_opt = findContainingNodes(block);
        containing_opt.has_value()) {
      if (containing_opt->empty()) {
        OUTCOME_TRY(append(block));
      } else {
        introduceBranch(*containing_opt, block);
      }
    } else {
      // this entry already exists
    }

    // update cumulative vote data.
    // NOTE: below this point, there always exists a node with the given hash
    // and number.
    BlockHash inspecting_hash = block.hash;
    while (true) {
      Entry &active_entry = entries_.at(inspecting_hash);
      active_entry.cumulative_vote.set(vote_type, index, weight);
      auto parent_it = active_entry.ancestors.rbegin();
      if (parent_it != active_entry.ancestors.rend()) {
        inspecting_hash = *parent_it;
      } else {
        break;
      }
    }

    return outcome::success();
  }

  void VoteGraphImpl::remove(VoteType vote_type, const Id &voter) {
    auto inw_res = voter_set_->indexAndWeight(voter);
    if (inw_res.has_value()) {
      const auto [index, weight] = inw_res.value();
      for (auto &[_, entry] : entries_) {
        entry.cumulative_vote.unset(vote_type, index, weight);
      }
    }
  }

  outcome::result<void> VoteGraphImpl::append(const BlockInfo &block) {
    if (base_.hash == block.hash) {
      return outcome::success();
    }
    if (base_.number > block.number) {
      return VoteGraphError::RECEIVED_BLOCK_LESS_THAN_BASE;
    }

    OUTCOME_TRY(ancestry, chain_->getAncestry(base_.hash, block.hash));

    BOOST_ASSERT_MSG(not ancestry.empty(),
                     "ancestry always contains at least 1 element - base");
    BOOST_ASSERT_MSG(
        ancestry.front() == block.hash,
        "ancestry always contains provided block as the first element");
    BOOST_ASSERT_MSG(ancestry.back() == base_.hash,
                     "ancestry always contains base block as the last element");

    auto entry_it = entries_.end();

    // Find first (best) ancestor among those presented in the entries,
    // and take corresponding entry
    auto ancestry_it = std::find_if(ancestry.begin() + 1,
                                    ancestry.end(),
                                    [this, &entry_it](auto &ancestor) {
                                      return entry_it = entries_.find(ancestor),
                                             entry_it != entries_.end();
                                    });
    BOOST_ASSERT_MSG(ancestry_it != ancestry.end(),
                     "at least one entry presents ancestor of appending block");

    // Found entry is got block as descendant
    if (entry_it != entries_.end()) {
      Entry &entry = entry_it->second;
      entry.descendants.push_back(block.hash);
    }

    // Needed ancestries is ancestries from parent to ancestor represented in
    // found entry
    std::vector<BlockHash> ancestors(ancestry.begin() + 1, ancestry_it + 1);

    // Block will become a head instead his oldest ancestor
    if (!ancestors.empty()) {
      BlockHash ancestor_hash = ancestors.back();
      heads_.erase(ancestor_hash);
      heads_.insert(block.hash);
    }

    // New entry is got ancestors and added to entries container
    Entry newEntry;
    newEntry.number = block.number;
    newEntry.ancestors = std::move(ancestors);
    entries_.insert({block.hash, std::move(newEntry)});

    return outcome::success();
  }

  void VoteGraphImpl::introduceBranch(
      const std::vector<primitives::BlockHash> &descendants,
      const BlockInfo &ancestor) {
    Entry new_entry;
    new_entry.number = ancestor.number;
    bool filled = false;  // is newEntry.ancestors filled
    std::optional<BlockHash> prev_ancestor_opt;

    for (const auto &descendant : descendants) {
      Entry &entry = entries_.at(descendant);

      BOOST_ASSERT_MSG(inDirectAncestry(entry, ancestor.hash, ancestor.number),
                       "not in direct ancestry");
      BOOST_ASSERT_MSG(ancestor.number <= entry.number,
                       "this function only invoked with direct ancestors; qed");

      size_t offset_size = entry.number - ancestor.number;
      auto ancestors_copy = entry.ancestors;
      BOOST_ASSERT(entry.ancestors.begin() + offset_size
                   < entry.ancestors.end());

      if (not filled) {
        // `[offset_size..end)` elements
        new_entry.ancestors = std::vector<BlockHash>{
            entry.ancestors.begin() + offset_size, entry.ancestors.end()};

        auto last_it = entry.ancestors.rbegin();
        if (last_it != entry.ancestors.rend()) {
          prev_ancestor_opt = *last_it;
        } else {
          prev_ancestor_opt = std::nullopt;
        }
        filled = true;
      }

      // `[0..offset_size)` elements
      entry.ancestors = std::vector<BlockHash>{
          entry.ancestors.begin(), entry.ancestors.begin() + offset_size};

      new_entry.descendants.push_back(descendant);

      new_entry.cumulative_vote.merge(entry.cumulative_vote, voter_set_);
    }

    if (prev_ancestor_opt) {
      auto it = entries_.find(*prev_ancestor_opt);
      BOOST_ASSERT_MSG(it != entries_.end(),
                       "Prior ancestor is referenced from a node; qed");

      auto &prev_ancestor_entry = it->second;
      std::unordered_set<BlockHash> set{new_entry.descendants.begin(),
                                        new_entry.descendants.end()};

      // filter descendants
      filter_if(prev_ancestor_entry.descendants,
                [&set](const BlockHash &hash) { return !contains(set, hash); });
      prev_ancestor_entry.descendants.push_back(ancestor.hash);
    }

    entries_.insert({ancestor.hash, std::move(new_entry)});
  }

  std::optional<BlockInfo> VoteGraphImpl::findGhost(
      VoteType vote_type,
      const std::optional<BlockInfo> &current_best,
      const VoteGraph::Condition &condition) const {
    bool force_constrain = false;
    BlockHash node_key = base_.hash;

    if (current_best.has_value()) {
      auto containing_opt = findContainingNodes(current_best.value());
      if (containing_opt.has_value()) {
        auto &containing = containing_opt.value();
        if (containing.empty()) {
          return std::nullopt;
        }

        const Entry &entry = entries_.at(containing[0]);
        auto ancestor_it = std::rbegin(entry.ancestors);
        BOOST_ASSERT_MSG(
            ancestor_it != std::rend(entry.ancestors),
            "node containing non-node in history always has ancestor; qed");
        node_key = *ancestor_it;
        force_constrain = true;
      } else {
        node_key = current_best->hash;
        force_constrain = false;
      }
    }

    Entry active_node = entries_.at(node_key);
    if (not condition(active_node.cumulative_vote)) {
      return std::nullopt;
    }

    /// entries to be processed
    std::stack<std::reference_wrapper<const Entry>> nodes;

    nodes.push(active_node);
    while (not nodes.empty()) {
      auto &node = nodes.top().get();
      nodes.pop();

      for (auto &descendant_hash : node.descendants) {
        auto &descendant = entries_.at(descendant_hash);

        if (force_constrain and current_best) {
          if (not inDirectAncestry(
                  descendant, current_best->hash, current_best->number)) {
            continue;
          }
        }
        if (not condition(descendant.cumulative_vote)) {
          continue;
        }

        if (descendant.number > active_node.number
            or (descendant.number == active_node.number
                and active_node.cumulative_vote.sum(vote_type)
                        < descendant.cumulative_vote.sum(vote_type))) {
          node_key = descendant_hash;
          active_node = descendant;

          nodes.push(descendant);
        }
      }

      force_constrain = false;
    }

    std::optional<BlockInfo> info =
        force_constrain ? current_best : std::nullopt;

    Subchain subchain =
        ghostFindMergePoint(vote_type, node_key, active_node, info, condition);
    auto &hashes = subchain.hashes;

    if (hashes.empty()) {
      return std::nullopt;
    }

    // return last hash with best number
    return BlockInfo(subchain.best_number, hashes.back());
  }

  VoteGraph::Subchain VoteGraphImpl::ghostFindMergePoint(
      VoteType vote_type,
      const BlockHash &active_node_hash,
      const VoteGraph::Entry &active_node,
      const std::optional<BlockInfo> &force_constrain,
      const VoteGraph::Condition &condition) const {
    auto descendants = active_node.descendants;
    filter_if(descendants, [&](const BlockHash &hash) {
      if (not force_constrain) {
        return true;
      }
      return inDirectAncestry(
          entries_.at(hash), force_constrain->hash, force_constrain->number);
    });

    const auto base_number = active_node.number;
    auto best_number = active_node.number;
    std::vector<BlockHash> hashes{active_node_hash};

    std::unordered_map<BlockHash, VoteWeight> descendant_blocks;

    size_t offset = 0;
    while (true) {
      std::optional<BlockHash> new_best;
      std::optional<VoteWeight> new_best_vote_weight;

      ++offset;
      for (const auto &d_node : descendants) {
        const Entry &entry = entries_.at(d_node);
        auto ancestor_opt = entry.getAncestorBlockBy(base_number + offset);
        if (not ancestor_opt.has_value()) {
          continue;
        }

        BlockHash &d_block = ancestor_opt.value();
        auto it = descendant_blocks.find(d_block);
        if (it == descendant_blocks.end()) {
          // if not found, insert then
          descendant_blocks[d_block] = entry.cumulative_vote;
        } else {
          // if found, update weight
          descendant_blocks[d_block].merge(entry.cumulative_vote, voter_set_);

          // check if block fulfills condition
          if (condition(descendant_blocks[d_block])) {
            if (not new_best_vote_weight
                or new_best_vote_weight->sum(vote_type)
                       < descendant_blocks[d_block].sum(vote_type)) {
              // we found our best block
              new_best = d_block;
              new_best_vote_weight = descendant_blocks[d_block];
            }
          }
        }
      }

      if (not new_best) {
        break;
      }

      ++best_number;
      descendant_blocks.clear();
      filter_if(descendants, [&](const BlockHash &hash) {
        return inDirectAncestry(entries_.at(hash), *new_best, best_number);
      });

      hashes.push_back(*new_best);
    }

    return Subchain{hashes, best_number};
  }

  void VoteGraphImpl::adjustBase(const std::vector<BlockHash> &ancestry_proof) {
    if (ancestry_proof.empty()) {
      return;
    }

    // take last hash
    auto new_hash = *std::rbegin(ancestry_proof);

    if (ancestry_proof.size() > base_.number) {
      return;
    }

    Entry &old_entry = entries_.at(base_.hash);
    old_entry.ancestors.insert(old_entry.ancestors.end(),
                               ancestry_proof.begin(),
                               ancestry_proof.end());

    primitives::BlockNumber new_number = base_.number - ancestry_proof.size();

    Entry new_entry;
    new_entry.number = new_number;
    new_entry.descendants.push_back(base_.hash);
    new_entry.cumulative_vote = old_entry.cumulative_vote;

    entries_[new_hash] = std::move(new_entry);
    base_ = BlockInfo{new_number, new_hash};
  }

  std::optional<BlockInfo> VoteGraphImpl::findAncestor(
      VoteType vote_type,
      const BlockInfo &block_arg,
      const VoteGraph::Condition &condition) const {
    for (auto block = block_arg;;) {
      auto nodes_opt = findContainingNodes(block);

      if (not nodes_opt.has_value()) {
        // The block has a vote-node in the graph.
        const Entry &node = entries_.at(block.hash);

        // If the condition is fulfilled, we are done.
        if (condition(node.cumulative_vote)) {
          return block;
        }

        // Not enough weight, check the parent block.
        if (node.ancestors.empty()) {
          return std::nullopt;
        } else {
          block.hash = node.ancestors[0];
          block.number = node.number - 1;
        }
      } else {
        const auto &children = nodes_opt.value();

        // If there are no vote-nodes below the block in the graph,
        // the block is not in the graph at all.
        if (children.empty()) {
          return std::nullopt;
        }

        // The block is "contained" in the graph (i.e. in the ancestry-chain
        // of at least one vote-node) but does not itself have a vote-node.
        VoteWeight cumulative_weight;
        for (auto &child : children) {
          const Entry &child_node = entries_.at(child);

          cumulative_weight.merge(child_node.cumulative_vote, voter_set_);
        }

        // Check if the accumulated weight on all child vote-nodes is
        // sufficient.
        if (condition(cumulative_weight)) {
          return block;
        }

        // Not enough weight, check the parent vote-node.
        const auto &child = children.back();
        const Entry &child_node = entries_.at(child);
        size_t offset = child_node.number - block.number;
        if (offset < child_node.ancestors.size()) {
          block.hash = child_node.ancestors[offset];
          block.number = child_node.number - offset - 1;
        } else {
          return std::nullopt;
        }
      }
    }
  }
}  // namespace kagome::consensus::grandpa
