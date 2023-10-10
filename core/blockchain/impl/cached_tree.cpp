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
      if (auto node = find(leaf)) {
        candidates.emplace(std::move(node));
      }
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

  void CachedTree::updateTreeRoot(std::shared_ptr<TreeNode> new_trie_root) {
    auto prev_root = root_;
    auto prev_node = new_trie_root->parent();

    // now node won't be deleted while cleaning children
    root_ = std::move(new_trie_root);

    // cleanup children from child to parent, because otherwise
    // when they are cleaned up when their parent shared ptr is deleted,
    // recursive calls of shared pointer destructors break the stack
    while (prev_node && prev_node != prev_root) {
      prev_node->children.clear();
      prev_node = prev_node->parent();
    }

    root_->weak_parent.reset();
  }

  const TreeNode &CachedTree::getRoot() const {
    BOOST_ASSERT(root_ != nullptr);
    return *root_;
  }

  TreeNode &CachedTree::getRoot() {
    BOOST_ASSERT(root_ != nullptr);
    return *root_;
  }

  void CachedTree::updateMeta(const std::shared_ptr<TreeNode> &new_node) {
    auto parent = wptrMustLock(new_node->weak_parent);
    parent->children.push_back(new_node);

    leaves_.insert(new_node->info.hash);
    leaves_.erase(parent->info.hash);
    chooseBest(new_node);
  }

  void CachedTree::removeFromMeta(const std::shared_ptr<TreeNode> &node) {
    auto parent = node->parent();
    if (parent == nullptr) {
      // Already removed with removed subtree
      return;
    }

    auto it = std::find(parent->children.begin(), parent->children.end(), node);
    if (it != parent->children.end()) {
      parent->children.erase(it);
    }

    leaves_.erase(node->info.hash);
    if (parent->children.empty()) {
      leaves_.insert(parent->info.hash);
    }

    if (node == best_) {
      best_ = parent;
      for (auto it = leaves_.begin(); it != leaves_.end();) {
        const auto &hash = *it++;
        const auto leaf_node = find(hash);
        if (leaf_node == nullptr) {
          // Already removed with removed subtree
          leaves_.erase(hash);
        } else if (chooseBest(leaf_node)) {
          break;
        }
      }
    }
  }

  CachedTree::CachedTree(const primitives::BlockInfo &root)
      : root_{std::make_shared<TreeNode>(root)},
        best_{root_},
        leaves_{root.hash} {}

  const std::shared_ptr<TreeNode> &CachedTree::finalized() const {
    return root_;
  }

  const std::shared_ptr<TreeNode> &CachedTree::best() const {
    return best_;
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
      if (auto node = find(leaf)) {
        candidates.emplace(std::move(node));
      }
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
    std::queue<std::shared_ptr<TreeNode>> queue;
    queue.push(root_);
    while (not queue.empty()) {
      auto &node = queue.front();
      if (node->info.hash == hash) {
        return node;
      }
      for (auto &child : node->children) {
        queue.push(child);
      }
      queue.pop();
    }
    return nullptr;
  }
}  // namespace kagome::blockchain
