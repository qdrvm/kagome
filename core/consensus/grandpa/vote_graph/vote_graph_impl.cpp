/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"

#include <functional>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/find_if.hpp>

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

  boost::optional<std::vector<primitives::BlockHash>>
  VoteGraphImpl::findContainingNodes(const BlockInfo &block) const {
    if (contains(entries_, block.block_hash)) {
      return boost::none;
    }

    std::vector<BlockHash> containing;
    std::unordered_set<BlockHash> visited;

    // iterate vote-heads and their ancestry backwards until we find the one
    // with this target hash in that chain.
    for (const auto &headHash : heads_) {
      BlockHash current = headHash;
      while (true) {
        auto active = entries_.find(current);
        if (active == entries_.end()) {
          break;
        }

        // if node has been checked already, break
        if (auto [_, inserted] = visited.insert(current); !inserted) {
          break;
        }

        auto ancestorOpt =
            active->second.getAncestorBlockBy(block.block_number);
        if (ancestorOpt && *ancestorOpt == block.block_hash) {
          containing.push_back(current);
        } else if (ancestorOpt && *ancestorOpt != block.block_hash) {
          // nothing in this branch. continue search.
        } else {
          const Entry &activeEntry = active->second;
          auto lastAncestorIt = activeEntry.ancestors.rbegin();
          if (lastAncestorIt != activeEntry.ancestors.rend()) {
            // iterate backwards
            current = *lastAncestorIt;
            continue;
          }
        }

        break;
      }
    }

    return containing;
  }

  outcome::result<void> VoteGraphImpl::insert(const BlockInfo &block,
                                              const VoteWeight &vote) {
    if (auto containingOpt = findContainingNodes(block); containingOpt) {
      if (containingOpt->empty()) {
        OUTCOME_TRY(append(block));
      } else {
        introduceBranch(*containingOpt, block);
      }
    } else {
      // this entry already exists
    }

    // update cumulative vote data.
    // NOTE: below this point, there always exists a node with the given hash
    // and number.
    BlockHash inspectingHash = block.block_hash;
    while (true) {
      Entry &activeEntry = entries_.at(inspectingHash);
      activeEntry.cumulative_vote += vote;
      auto parentIt = activeEntry.ancestors.rbegin();
      if (parentIt != activeEntry.ancestors.rend()) {
        inspectingHash = *parentIt;
      } else {
        break;
      }
    }

    return outcome::success();
  }

  outcome::result<void> VoteGraphImpl::append(const BlockInfo &block) {
    OUTCOME_TRY(ancestry,
                chain_->getAncestry(base_.block_hash, block.block_hash));
    ancestry.push_back(base_.block_hash);

    size_t ancestorIndex = 0;
    for (size_t i = 0u, size = ancestry.size(); i < size; ++i) {
      auto &ancestor = ancestry[i];
      if (auto entryIt = entries_.find(ancestor); entryIt != entries_.end()) {
        Entry &entry = entryIt->second;
        entry.descendents.push_back(block.block_hash);
        ancestorIndex = i;
        break;
      }
    }

    BOOST_ASSERT_MSG(!ancestry.empty(),
                     "ancestry always contains at least 1 element - base");

    BlockHash ancestorHash = ancestry[ancestorIndex];
    ancestry.resize(ancestorIndex + 1);

    Entry newEntry;
    newEntry.number = block.block_number;
    newEntry.ancestors = ancestry;

    entries_.insert({block.block_hash, std::move(newEntry)});
    heads_.erase(ancestorHash);
    heads_.insert(block.block_hash);

    return outcome::success();
  }

  void VoteGraphImpl::introduceBranch(
      const std::vector<primitives::BlockHash> &descendents,
      const BlockInfo &ancestor) {
    Entry newEntry;
    newEntry.number = ancestor.block_number;
    bool filled = false;  // is newEntry.ancestors filled
    boost::optional<BlockHash> prevAncestorOpt;

    for (const auto &descendent : descendents) {
      Entry &entry = entries_.at(descendent);

      BOOST_ASSERT_MSG(
          inDirectAncestry(entry, ancestor.block_hash, ancestor.block_number),
          "not in direct ancestry");
      BOOST_ASSERT_MSG(ancestor.block_number <= entry.number,
                       "this function only invoked with direct ancestors; qed");

      size_t offset_size = entry.number - ancestor.block_number;
      auto ancestors_copy = entry.ancestors;
      BOOST_ASSERT(entry.ancestors.begin() + offset_size
                   < entry.ancestors.end());

      if (!filled) {
        // `[offset_size..end)` elements
        newEntry.ancestors = std::vector<BlockHash>{
            entry.ancestors.begin() + offset_size, entry.ancestors.end()};

        auto lastIt = entry.ancestors.rbegin();
        if (lastIt != entry.ancestors.rend()) {
          prevAncestorOpt = *lastIt;
        } else {
          prevAncestorOpt = boost::none;
        }
        filled = true;
      }

      // `[0..offset_size)` elements
      entry.ancestors = std::vector<BlockHash>{
          entry.ancestors.begin(), entry.ancestors.begin() + offset_size};

      newEntry.descendents.push_back(descendent);
      newEntry.cumulative_vote += entry.cumulative_vote;
    }

    if (prevAncestorOpt) {
      auto it = entries_.find(*prevAncestorOpt);
      BOOST_ASSERT_MSG(it != entries_.end(),
                       "Prior ancestor is referenced from a node; qed");

      auto &prev_ancestor_entry = it->second;
      std::unordered_set<BlockHash> set{newEntry.descendents.begin(),
                                        newEntry.descendents.end()};

      // filter descendents
      filter_if(prev_ancestor_entry.descendents,
                [&set](const BlockHash &hash) { return !contains(set, hash); });
      prev_ancestor_entry.descendents.push_back(ancestor.block_hash);
    }

    entries_.insert({ancestor.block_hash, std::move(newEntry)});
  }

  boost::optional<BlockInfo> VoteGraphImpl::findGhost(
      const boost::optional<BlockInfo> &current_best,
      const VoteGraph::Condition &condition) const {
    bool force_constrain = false;
    BlockHash node_key = base_.block_hash;

    if (current_best) {
      auto containingOpt = findContainingNodes(*current_best);
      if (containingOpt) {
        auto &containing = *containingOpt;
        if (containing.empty()) {
          return boost::none;
        }

        const Entry &entry = entries_.at(containing[0]);
        auto ancestorIt = std::rbegin(entry.ancestors);
        BOOST_ASSERT_MSG(
            ancestorIt != std::rend(entry.ancestors),
            "node containing non-node in history always has ancestor; qed");
        node_key = *ancestorIt;
        force_constrain = true;
      } else {
        node_key = current_best->block_hash;
        force_constrain = false;
      }
    }

    Entry active_node = entries_.at(node_key);
    if (!condition(active_node.cumulative_vote)) {
      return boost::none;
    }

    // breadth-first search starting from this node.
    bool loop = true;
    while (loop) {
      //            for (const BlockHash &descendent : active_node->descendents)
      //            {
      //              Entry &entry = entries_.at(descendent);
      //
      //              // take only descendents with our block in the ancestry.
      //              // a-la filter for given descendants vector
      //              if (force_constrain && current_best) {
      //                auto &best = *current_best;
      //                if (!inDirectAncestry(entry, best.block_hash,
      //                best.block_number)) {
      //                  continue;
      //                }
      //              }
      //
      //              if (condition(entry.cumulative_vote)) {
      //                force_constrain = false;
      //                node_key = descendent;  // hash for given entry
      //                active_node = &entry;
      //                break;
      //              } else {
      //                loop = false;
      //                break;
      //              }
      //            }

      using namespace boost::adaptors;
      auto filtered_entries =
          active_node.descendents | transformed([this](const BlockHash &d) {
            return std::make_pair(d, entries_.at(d));
          })
          | filtered([&](const auto &pair) {
              const auto &[_, node] = pair;
              if (force_constrain && current_best) {
                return inDirectAncestry(
                    node, current_best->block_hash, current_best->block_number);
              }
              return true;
            });
      const auto &next_descendent_it = boost::range::find_if(
          filtered_entries, [&condition](const auto &pair) {
            const auto &[_, node] = pair;
            return condition(node.cumulative_vote);
          });
      if (next_descendent_it == filtered_entries.end()) {
        break;
      }
      force_constrain = false;
      const auto &[key, node] = *next_descendent_it;
      node_key = key;
      active_node = node;
    }

    boost::optional<BlockInfo> info =
        force_constrain ? current_best : boost::none;

    Subchain subchain =
        ghostFindMergePoint(node_key, active_node, info, condition);
    auto &h = subchain.hashes;

    if (h.empty()) {
      return boost::none;
    }

    // return last hash with best number
    return BlockInfo(subchain.best_number, h[h.size() - 1]);
  }

  VoteGraph::Subchain VoteGraphImpl::ghostFindMergePoint(
      const BlockHash &active_node_hash,
      const VoteGraph::Entry &active_node,
      const boost::optional<BlockInfo> &force_constrain,
      const VoteGraph::Condition &condition) const {
    auto descendents = active_node.descendents;
    filter_if(descendents, [&](const BlockHash &hash) {
      if (!force_constrain) {
        return true;
      }
      return inDirectAncestry(entries_.at(hash),
                              force_constrain->block_hash,
                              force_constrain->block_number);
    });

    const auto base_number = active_node.number;
    auto best_number = active_node.number;
    std::vector<BlockHash> hashes{active_node_hash};

    std::unordered_map<BlockHash, VoteWeight> descendent_blocks;

    size_t offset = 0;
    while (true) {
      boost::optional<BlockHash> new_best;

      ++offset;
      for (const auto &d_node : descendents) {
        const Entry &entry = entries_.at(d_node);
        auto ancestorOpt = entry.getAncestorBlockBy(base_number + offset);
        if (!ancestorOpt) {
          continue;
        }

        BlockHash &d_block = *ancestorOpt;
        auto it = descendent_blocks.find(d_block);
        if (it == descendent_blocks.end()) {
          // if not found, insert then
          descendent_blocks[d_block] = entry.cumulative_vote;
        } else {
          // if found, update weight
          descendent_blocks[d_block] += entry.cumulative_vote;
          // check if block fullfills condition
          if (condition(descendent_blocks[d_block])) {
            // we found our best block
            new_best = d_block;
            break;
          }
        }  // end if
      }    // end for

      if (!new_best) {
        break;
      }

      ++best_number;
      descendent_blocks.clear();
      filter_if(descendents, [&](const BlockHash &hash) {
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

    if (ancestry_proof.size() > base_.block_number) {
      return;
    }

    Entry &old_entry = entries_.at(base_.block_hash);
    old_entry.ancestors.insert(old_entry.ancestors.end(),
                               ancestry_proof.begin(),
                               ancestry_proof.end());

    auto new_number = base_.block_number - ancestry_proof.size();

    Entry newEntry;
    newEntry.number = new_number;
    newEntry.descendents.push_back(base_.block_hash);
    newEntry.cumulative_vote = old_entry.cumulative_vote;

    entries_[new_hash] = std::move(newEntry);
    base_ = BlockInfo{new_number, new_hash};
  }

  boost::optional<BlockInfo> VoteGraphImpl::findAncestor(
      const BlockInfo &block, const VoteGraph::Condition &condition) const {
    // we store two nodes with an edge between them that is the canonical
    // chain.
    // the `node_key` always points to the ancestor node, and the
    // `canonical_node` points to the higher node.
    boost::optional<Entry> canonical_node = boost::none;
    BlockHash node_key;

    auto nodesOpt = findContainingNodes(block);
    if (!nodesOpt) {
      const Entry &entry = entries_.at(block.block_hash);
      if (condition(entry.cumulative_vote)) {
        return block;
      }

      auto ancestorIt = std::rbegin(entry.ancestors);
      if (ancestorIt == std::rend(entry.ancestors)) {
        return boost::none;
      }

      canonical_node = entry;
      node_key = *ancestorIt;
    } else {
      auto &nodes = *nodesOpt;
      if (nodes.empty()) {
        return boost::none;
      }

      const Entry entry = entries_.at(nodes[0]);
      auto ancIt = std::rbegin(entry.ancestors);
      BOOST_ASSERT_MSG(
          ancIt != std::rend(entry.ancestors),
          "node containing block in ancestry has ancestor node; qed");

      canonical_node = entry;
      node_key = *ancIt;
    }

    BOOST_ASSERT(canonical_node != boost::none);

    // search backwards until we find the first vote-node that
    // meets the condition.
    Entry active_node = entries_.at(node_key);
    while (!condition(active_node.cumulative_vote)) {
      auto ancestorIt = active_node.ancestors.rbegin();
      if (ancestorIt == active_node.ancestors.rend()) {
        return boost::none;
      }

      node_key = *ancestorIt;
      canonical_node = active_node;
      active_node = entries_.at(node_key);
    }

    // find the GHOST merge-point after the active_node.
    // constrain it to be within the canonical chain.
    auto good_subchain =
        ghostFindMergePoint(node_key, active_node, boost::none, condition);

    BOOST_ASSERT(canonical_node);
    // search in reverse order
    auto &hashes = good_subchain.hashes;
    auto bestHashIt =
        std::find_if(hashes.rbegin(),
                     hashes.rend(),
                     [&canonical_node, number = good_subchain.best_number](
                         const BlockHash &hash) {
                       return inDirectAncestry(*canonical_node, hash, number);
                     });
    if (bestHashIt == hashes.rend()) {
      // not found
      return boost::none;
    }

    return BlockInfo{good_subchain.best_number, *bestHashIt};
  }

  VoteGraphImpl::VoteGraphImpl(const BlockInfo &base,
                               std::shared_ptr<Chain> chain)
      : chain_(std::move(chain)), base_(base) {
    Entry entry;
    entry.number = base.block_number;

    entries_[base.block_hash] = std::move(entry);

    heads_.insert(base.block_hash);
  }
}  // namespace kagome::consensus::grandpa
