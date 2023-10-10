/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/cached_tree.hpp"

#include <queue>
#include <set>

#include "utils/wptr.hpp"

namespace kagome::blockchain {
  bool Reorg::empty() const {
    return revert.empty() and apply.empty();
  }

  TreeNode::TreeNode(const primitives::BlockInfo &info)
      : info{info},
        babe_primary_weight{0},
        contains_approved_para_block{false},
        reverted{false} {}

  TreeNode::TreeNode(const primitives::BlockInfo &info,
                     const std::shared_ptr<TreeNode> &parent,
                     bool babe_primary)
      : info{info},
        weak_parent{parent},
        babe_primary_weight{parent->babe_primary_weight
                            + (babe_primary ? 1 : 0)},
        contains_approved_para_block{false},
        reverted{parent->reverted} {}

  std::shared_ptr<TreeNode> TreeNode::parent() const {
    return wptrLock(weak_parent);
  }

  BlockWeight TreeNode::weight() const {
    return {babe_primary_weight, info.number};
  }

  Reorg reorg(std::shared_ptr<TreeNode> from, std::shared_ptr<TreeNode> to) {
    Reorg reorg;
    while (from != to) {
      if (from->info.number > to->info.number) {
        reorg.revert.emplace_back(from->info);
        from = wptrMustLock(from->weak_parent);
      } else {
        reorg.apply.emplace_back(to->info);
        to = wptrMustLock(to->weak_parent);
      }
    }
    reorg.common = to->info;
    std::reverse(reorg.apply.begin(), reorg.apply.end());
    return reorg;
  }

  template <typename F>
  bool descend(std::shared_ptr<TreeNode> from,
               const std::shared_ptr<TreeNode> &to,
               const F &f) {
    while (from != to) {
      if (from->info.number <= to->info.number) {
        return false;
      }
      f(from);
      from = wptrMustLock(from->weak_parent);
    }
    return true;
  }

  bool canDescend(std::shared_ptr<TreeNode> from,
                  const std::shared_ptr<TreeNode> &to) {
    return descend(from, to, [](const std::shared_ptr<TreeNode>) {});
  }

  bool CachedTree::chooseBest(std::shared_ptr<TreeNode> node) {
    if (node->reverted) {
      return false;
    }
    BOOST_ASSERT(not best_->reverted);
    if (node->weight() > best_->weight()) {
      best_ = node;
      return true;
    }
    return false;
  }

  struct Cmp {
    bool operator()(const std::shared_ptr<const TreeNode> &lhs,
                    const std::shared_ptr<const TreeNode> &rhs) const {
      BOOST_ASSERT(lhs and rhs);
      return lhs->info > rhs->info;
    }
  };

  void CachedTree::forceRefreshBest() {
    std::set<std::shared_ptr<TreeNode>, Cmp> candidates;
    for (auto &leaf : leaves_) {
      auto node = find(leaf);
      BOOST_ASSERT(node);
      candidates.emplace(std::move(node));
    }

    best_ = root_;
    while (not candidates.empty()) {
      auto node = candidates.extract(candidates.begin());
      BOOST_ASSERT(not node.empty());

      auto &tree_node = node.value();
      if (tree_node->reverted) {
        if (auto parent = tree_node->parent()) {
          candidates.emplace(std::move(parent));
        }
        continue;
      }

      if (best_->weight() < tree_node->weight()) {
        best_ = tree_node;
      }
    }
  }

  CachedTree::CachedTree(const primitives::BlockInfo &root)
      : root_{std::make_shared<TreeNode>(root)},
        best_{root_},
        nodes_{{root.hash, root_}},
        leaves_{root.hash} {}

  primitives::BlockInfo CachedTree::finalized() const {
    return root_->info;
  }

  primitives::BlockInfo CachedTree::best() const {
    return best_->info;
  }

  size_t CachedTree::leafCount() const {
    return leaves_.size();
  }

  std::vector<primitives::BlockHash> CachedTree::leafHashes() const {
    return {leaves_.begin(), leaves_.end()};
  }

  bool CachedTree::isLeaf(const primitives::BlockHash &hash) const {
    return leaves_.find(hash) != leaves_.end();
  }

  primitives::BlockInfo CachedTree::bestWith(
      const std::shared_ptr<TreeNode> &required) const {
    std::set<std::shared_ptr<TreeNode>, Cmp> candidates;
    for (auto &leaf : leaves_) {
      auto node = find(leaf);
      BOOST_ASSERT(node);
      candidates.emplace(std::move(node));
    }
    auto best = required;
    while (not candidates.empty()) {
      auto _node = candidates.extract(candidates.begin());
      auto &node = _node.value();
      if (node->info.number <= required->info.number) {
        continue;
      }
      if (node->reverted) {
        if (auto parent = node->parent()) {
          candidates.emplace(std::move(parent));
        }
        continue;
      }
      if (node->weight() > best->weight() and canDescend(node, required)) {
        best = node;
      }
    }
    return best->info;
  }

  std::shared_ptr<TreeNode> CachedTree::find(
      const primitives::BlockHash &hash) const {
    if (auto it = nodes_.find(hash); it != nodes_.end()) {
      return it->second;
    }
    return nullptr;
  }

  std::optional<Reorg> CachedTree::add(
      const std::shared_ptr<TreeNode> &new_node) {
    if (nodes_.find(new_node->info.hash) != nodes_.end()) {
      return std::nullopt;
    }
    BOOST_ASSERT(new_node->children.empty());
    auto parent = wptrMustLock(new_node->weak_parent);
    auto child_it =
        std::find(parent->children.begin(), parent->children.end(), new_node);
    BOOST_ASSERT(child_it == parent->children.end());
    parent->children.emplace_back(new_node);
    nodes_.emplace(new_node->info.hash, new_node);
    leaves_.erase(parent->info.hash);
    leaves_.emplace(new_node->info.hash);
    if (not new_node->reverted and new_node->weight() > best_->weight()) {
      auto old_best = best_;
      best_ = new_node;
      return reorg(old_best, best_);
    }
    return std::nullopt;
  }

  ReorgAndPrune CachedTree::finalize(
      const std::shared_ptr<TreeNode> &new_finalized) {
    BOOST_ASSERT(new_finalized->info.number >= root_->info.number);
    if (new_finalized == root_) {
      return {};
    }
    BOOST_ASSERT(new_finalized->parent());
    ReorgAndPrune changes;
    if (not canDescend(best_, new_finalized)) {
      changes.reorg = reorg(best_, new_finalized);
    }
    std::deque<std::shared_ptr<TreeNode>> queue;
    for (auto finalized_child = new_finalized,
              parent = finalized_child->parent();
         parent;
         finalized_child = parent, parent = parent->parent()) {
      for (auto &child : parent->children) {
        if (child == finalized_child) {
          continue;
        }
        queue.emplace_back(child);
      }
      parent->children.clear();
      nodes_.erase(parent->info.hash);
    }
    while (not queue.empty()) {
      auto parent = std::move(queue.front());
      queue.pop_front();
      changes.prune.emplace_back(parent->info);
      for (auto &child : parent->children) {
        queue.emplace_back(child);
      }
      if (parent->children.empty()) {
        leaves_.erase(parent->info.hash);
      }
      parent->children.clear();
      nodes_.erase(parent->info.hash);
    }
    std::reverse(changes.prune.begin(), changes.prune.end());
    root_ = new_finalized;
    root_->weak_parent.reset();
    if (changes.reorg) {
      forceRefreshBest();
      size_t offset = changes.reorg->apply.size();
      auto ok = descend(
          best_, new_finalized, [&](const std::shared_ptr<TreeNode> node) {
            changes.reorg->apply.emplace_back(node->info);
          });
      BOOST_ASSERT(ok);
      std::reverse(changes.reorg->apply.begin() + offset,
                   changes.reorg->apply.end());
    }
    return changes;
  }

  ReorgAndPrune CachedTree::removeLeaf(const primitives::BlockHash &hash) {
    ReorgAndPrune changes;
    auto node_it = nodes_.find(hash);
    BOOST_ASSERT(node_it != nodes_.end());
    auto &node = node_it->second;
    BOOST_ASSERT(node);
    auto leaf_it = leaves_.find(hash);
    BOOST_ASSERT(leaf_it != leaves_.end());
    BOOST_ASSERT(node->children.empty());
    auto parent = wptrMustLock(node->weak_parent);
    auto child_it =
        std::find(parent->children.begin(), parent->children.end(), node);
    BOOST_ASSERT(child_it != parent->children.end());
    changes.prune.emplace_back(node->info);
    parent->children.erase(child_it);
    if (parent->children.empty()) {
      leaves_.emplace(parent->info.hash);
    }
    leaves_.erase(leaf_it);
    if (node == best_) {
      auto old_best = node;
      forceRefreshBest();
      changes.reorg = reorg(old_best, best_);
    }
    nodes_.erase(node_it);
    return changes;
  }

  ReorgAndPrune CachedTree::removeUnfinalized() {
    ReorgAndPrune changes;
    if (best_ != root_) {
      changes.reorg = reorg(best_, root_);
    }
    std::deque<std::shared_ptr<TreeNode>> queue{root_};
    while (not queue.empty()) {
      auto parent = std::move(queue.front());
      queue.pop_front();
      for (auto &child : parent->children) {
        changes.prune.emplace_back(child->info);
        queue.emplace_back(child);
      }
      parent->children.clear();
    }
    std::reverse(changes.prune.begin(), changes.prune.end());
    *this = CachedTree{root_->info};
    return changes;
  }
}  // namespace kagome::blockchain
