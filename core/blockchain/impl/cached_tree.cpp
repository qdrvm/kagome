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
      : block_hash{info.hash},
        depth{info.number},
        babe_primary_weight{0},
        contains_approved_para_block{false},
        reverted{false} {}

  TreeNode::TreeNode(const primitives::BlockInfo &info,
                     const std::shared_ptr<TreeNode> &parent,
                     bool babe_primary)
      : block_hash{info.hash},
        depth{info.number},
        weak_parent{parent},
        babe_primary_weight{parent->babe_primary_weight
                            + (babe_primary ? 1 : 0)},
        contains_approved_para_block{false},
        reverted{parent->reverted} {}

  std::shared_ptr<TreeNode> TreeNode::parent() const {
    return wptrLock(weak_parent);
  }

  BlockWeight TreeNode::weight() const {
    return {babe_primary_weight, depth};
  }

  std::shared_ptr<const TreeNode> TreeNode::findByHash(
      const primitives::BlockHash &hash) const {
    // standard BFS
    std::queue<std::shared_ptr<const TreeNode>> nodes_to_scan;
    nodes_to_scan.push(shared_from_this());
    while (!nodes_to_scan.empty()) {
      const auto &node = nodes_to_scan.front();
      if (node->block_hash == hash) {
        return node;
      }
      for (const auto &child : node->children) {
        nodes_to_scan.push(child);
      }
      nodes_to_scan.pop();
    }
    return nullptr;
  }

  bool TreeNode::operator==(const TreeNode &other) const {
    return block_hash == other.block_hash && depth == other.depth;
  }

  bool TreeNode::operator!=(const TreeNode &other) const {
    return !(*this == other);
  }

  template <typename F>
  bool descend(std::shared_ptr<TreeNode> from,
               const std::shared_ptr<TreeNode> &to,
               const F &f) {
    while (from != to) {
      if (from->depth <= to->depth) {
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
      return std::tie(lhs->depth, lhs->block_hash)
           > std::tie(rhs->depth, rhs->block_hash);
    }
  };

  void CachedTree::forceRefreshBest() {
    std::set<std::shared_ptr<TreeNode>, Cmp> candidates;
    for (auto &leaf : leaves_) {
      if (auto node = root_->findByHash(leaf)) {
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

    leaves_.insert(new_node->block_hash);
    leaves_.erase(parent->block_hash);
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

    leaves_.erase(node->block_hash);
    if (parent->children.empty()) {
      leaves_.insert(parent->block_hash);
    }

    if (node == best_) {
      best_ = parent;
      for (auto it = leaves_.begin(); it != leaves_.end();) {
        const auto &hash = *it++;
        const auto leaf_node = root_->findByHash(hash);
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
      if (auto node = required->findByHash(leaf)) {
        candidates.emplace(std::move(node));
      }
    }
    auto best = required;
    while (not candidates.empty()) {
      auto _node = candidates.extract(candidates.begin());
      auto &node = _node.value();
      if (node->depth <= required->depth) {
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
    return best->getBlockInfo();
  }
}  // namespace kagome::blockchain
