/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/cached_tree.hpp"

#include <queue>

#include <iostream>
#include <set>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, TreeNode::Error, e) {
  using E = kagome::blockchain::TreeNode::Error;
  switch (e) {
    case E::NO_CHAIN_BETWEEN_BLOCKS:
      return "no chain exists between given blocks";
  }
  return "unknown error";
}

namespace kagome::blockchain {
  TreeNode::TreeNode(const primitives::BlockHash &hash,
                     primitives::BlockNumber depth)
      : block_hash{hash},
        depth{depth},
        parent{},
        finalized{true},
        babe_primary{false},
        contains_approved_para_block{false},
        reverted{false} {}

  TreeNode::TreeNode(const primitives::BlockHash &hash,
                     primitives::BlockNumber depth,
                     const std::shared_ptr<TreeNode> &parent,
                     bool finalized,
                     bool babe_primary)
      : block_hash{hash},
        depth{depth},
        parent{parent},
        finalized{finalized},
        babe_primary{babe_primary},
        contains_approved_para_block{false},
        reverted{parent ? parent->reverted : false} {}

  outcome::result<void> TreeNode::applyToChain(
      const primitives::BlockInfo &chain_end,
      const std::function<outcome::result<ExitToken>(const TreeNode &node)> &op)
      const {
    using ChildIdx = size_t;
    std::map<primitives::BlockHash, ChildIdx> fork_choice;

    const auto *current_node = findByHash(chain_end.hash).get();
    if (current_node == nullptr) {
      return Error::NO_CHAIN_BETWEEN_BLOCKS;
    }

    // mostly to catch typos in tests, but who knows
    BOOST_ASSERT(current_node->depth == chain_end.number);
    // now we must memorize where to go on forks in order to traverse
    // from this to chain_end
    while (current_node->depth > this->depth) {
      auto curren_parent = current_node->parent.lock();
      BOOST_ASSERT(curren_parent != nullptr);
      // if there's a fork at the parent, memorize which branch to take
      if (curren_parent->children.size() > 1) {
        const auto child_idx = std::find_if(curren_parent->children.begin(),
                                            curren_parent->children.end(),
                                            [current_node](auto &sptr) {
                                              return sptr.get() == current_node;
                                            })
                             - curren_parent->children.begin();
        fork_choice[curren_parent->block_hash] = child_idx;
      }
      current_node = curren_parent.get();
    }
    if (current_node->block_hash != this->block_hash) {
      return Error::NO_CHAIN_BETWEEN_BLOCKS;
    }

    current_node = this;
    do {
      OUTCOME_TRY(exit_token, op(*current_node));
      if (exit_token == ExitToken::EXIT) {
        return outcome::success();
      }
      if (current_node->children.size() == 1) {
        current_node = current_node->children.front().get();
      } else if (current_node->children.size() > 1) {
        auto child_idx = fork_choice[current_node->block_hash];
        current_node = current_node->children[child_idx].get();
      } else {
        break;
      }
    } while (current_node->depth <= chain_end.number);

    return outcome::success();
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
    const auto &other_parent = other.parent;
    auto parents_equal = (parent.expired() && other_parent.expired())
                      || (!parent.expired() && !other_parent.expired()
                          && parent.lock() == other_parent.lock());

    return parents_equal && block_hash == other.block_hash
        && depth == other.depth;
  }

  bool TreeNode::operator!=(const TreeNode &other) const {
    return !(*this == other);
  }

  TreeMeta::TreeMeta(const std::shared_ptr<TreeNode> &subtree_root_node)
      : best_block{subtree_root_node}, last_finalized{subtree_root_node} {
    std::function<void(std::shared_ptr<TreeNode>)> handle =
        [&](std::shared_ptr<TreeNode> node) {
          // avoid deep recursion
          while (node->children.size() == 1) {
            node = node->children.front();
          }

          // is a leaf
          if (node->children.empty()) {
            leaves.emplace(node->block_hash);
            chooseBest(node);
          } else {
            // follow descendants recursively
            for (const auto &child : node->children) {
              handle(child);
            }
          }
        };

    handle(subtree_root_node);
  }

  inline TreeMeta::Weight TreeMeta::getWeight(
      std::shared_ptr<TreeNode> node) const {
    auto finalized = last_finalized.lock();
    BOOST_ASSERT(finalized);
    Weight weight{WeightInfo(0ull), node->depth};
    while (node != finalized) {
      BOOST_ASSERT(node->depth > finalized->depth);
      if (node->babe_primary) {
        ++weight.first.data.babe_primary;
      }
      if (node->contains_approved_para_block) {
        ++weight.first.data.parachain_payload;
      }
      auto parent = node->parent.lock();
      BOOST_ASSERT(parent);
      node = std::move(parent);
    }
    return weight;
  }

  bool TreeMeta::chooseBest(std::shared_ptr<TreeNode> node) {
    if (node->reverted) {
      return false;
    }
    auto best = best_block.lock();
    BOOST_ASSERT(best);
    BOOST_ASSERT(not best->reverted);
    if (getWeight(node) > getWeight(best)) {
      best_block = node;
      return true;
    }
    return false;
  }

  void TreeMeta::forceRefreshBest() {
    auto root = last_finalized.lock();
    struct Cmp {
      bool operator()(const std::shared_ptr<const TreeNode> &lhs,
                      const std::shared_ptr<const TreeNode> &rhs) const {
        BOOST_ASSERT(lhs and rhs);
        return lhs->depth < rhs->depth
            or (lhs->depth == rhs->depth and lhs->block_hash < rhs->block_hash);
      }
    };

    std::set<std::shared_ptr<TreeNode>, Cmp> candidates;
    for (auto &leaf : leaves) {
      if (auto node = root->findByHash(leaf)) {
        candidates.emplace(std::move(node));
      }
    }

    auto best = std::move(root);
    while (not candidates.empty()) {
      auto node = candidates.extract((++candidates.rbegin()).base());
      BOOST_ASSERT(not node.empty());

      auto &tree_node = node.value();
      if (tree_node->reverted) {
        if (auto parent = tree_node->parent.lock()) {
          candidates.emplace(std::move(parent));
        }
        continue;
      }

      if (getWeight(best) < getWeight(tree_node)) {
        best = tree_node;
      }
    }

    best_block = best;
  }

  void CachedTree::updateTreeRoot(std::shared_ptr<TreeNode> new_trie_root) {
    auto prev_root = root_;
    auto prev_node = new_trie_root->parent.lock();

    // now node won't be deleted while cleaning children
    root_ = std::move(new_trie_root);

    // cleanup children from child to parent, because otherwise
    // when they are cleaned up when their parent shared ptr is deleted,
    // recursive calls of shared pointer destructors break the stack
    while (prev_node && prev_node != prev_root) {
      prev_node->children.clear();
      prev_node = prev_node->parent.lock();
    }

    metadata_ = std::make_shared<TreeMeta>(root_);
    root_->parent.reset();
  }

  const TreeNode &CachedTree::getRoot() const {
    BOOST_ASSERT(root_ != nullptr);
    return *root_;
  }

  TreeNode &CachedTree::getRoot() {
    BOOST_ASSERT(root_ != nullptr);
    return *root_;
  }

  const TreeMeta &CachedTree::getMetadata() const {
    BOOST_ASSERT(metadata_ != nullptr);
    return *metadata_;
  }

  void CachedTree::updateMeta(const std::shared_ptr<TreeNode> &new_node) {
    auto parent = new_node->parent.lock();
    parent->children.push_back(new_node);

    metadata_->leaves.insert(new_node->block_hash);
    metadata_->leaves.erase(parent->block_hash);
    metadata_->chooseBest(new_node);
  }

  void CachedTree::forceRefreshBest() {
    metadata_->forceRefreshBest();
  }

  void CachedTree::removeFromMeta(const std::shared_ptr<TreeNode> &node) {
    auto parent = node->parent.lock();
    if (parent == nullptr) {
      // Already removed with removed subtree
      return;
    }

    auto it = std::find(parent->children.begin(), parent->children.end(), node);
    if (it != parent->children.end()) {
      parent->children.erase(it);
    }

    metadata_->leaves.erase(node->block_hash);
    if (parent->children.empty()) {
      metadata_->leaves.insert(parent->block_hash);
    }

    auto best = metadata_->best_block.lock();
    BOOST_ASSERT(best);
    if (node == best) {
      metadata_->best_block = parent;
      for (auto it = metadata_->leaves.begin();
           it != metadata_->leaves.end();) {
        const auto &hash = *it++;
        const auto leaf_node = root_->findByHash(hash);
        if (leaf_node == nullptr) {
          // Already removed with removed subtree
          metadata_->leaves.erase(hash);
        } else if (metadata_->chooseBest(leaf_node)) {
          break;
        }
      }
    }
  }
}  // namespace kagome::blockchain
