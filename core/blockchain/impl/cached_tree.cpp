/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/cached_tree.hpp"

#include <queue>

#include <iostream>

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
                     primitives::BlockNumber depth,
                     consensus::EpochDigest &&curr_epoch_digest,
                     consensus::EpochNumber epoch_number,
                     consensus::EpochDigest &&next_epoch_digest,
                     bool finalized)
      : block_hash{hash},
        depth{depth},
        epoch_number(epoch_number),
        finalized{finalized} {
    this->epoch_digest =
        std::make_shared<consensus::EpochDigest>(std::move(curr_epoch_digest));
    this->next_epoch_digest = *epoch_digest == next_epoch_digest
                                  ? epoch_digest
                                  : std::make_shared<consensus::EpochDigest>(
                                      std::move(next_epoch_digest));
  }

  TreeNode::TreeNode(
      const primitives::BlockHash &hash,
      primitives::BlockNumber depth,
      const std::shared_ptr<TreeNode> &parent,
      consensus::EpochNumber epoch_number,
      std::optional<consensus::EpochDigest> next_epoch_digest_opt,
      bool finalized)
      : block_hash{hash},
        depth{depth},
        parent{parent},
        epoch_number(epoch_number),
        finalized{finalized} {
    BOOST_ASSERT(parent != nullptr or next_epoch_digest_opt.has_value());
    if (parent) {
      epoch_digest = epoch_number != parent->epoch_number
                         ? parent->next_epoch_digest
                         : epoch_digest = parent->epoch_digest;
      next_epoch_digest = parent->next_epoch_digest;
    } else {
      epoch_digest = std::make_shared<consensus::EpochDigest>(
          next_epoch_digest_opt.value());
      next_epoch_digest = epoch_digest;
    }

    if (next_epoch_digest_opt.has_value()) {
      if (next_epoch_digest_opt != *next_epoch_digest) {
        if (next_epoch_digest_opt == *epoch_digest) {
          next_epoch_digest = epoch_digest;
        } else {
          next_epoch_digest = std::make_shared<consensus::EpochDigest>(
              std::move(next_epoch_digest_opt.value()));
        }
      }
    }
  }

  outcome::result<void> TreeNode::applyToChain(
      const primitives::BlockInfo &chain_end,
      std::function<outcome::result<ExitToken>(TreeNode const &node)> const &op)
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
      : deepest_leaf{subtree_root_node}, last_finalized{subtree_root_node} {
    std::function<void(std::shared_ptr<TreeNode>)> handle =
        [&](std::shared_ptr<TreeNode> node) {
          // avoid deep recursion
          while (node->children.size() == 1) {
            node = node->children.front();
          }

          // is a leaf
          if (node->children.empty()) {
            leaves.emplace(node->block_hash);
            BOOST_ASSERT(  // NOLINT(bugprone-lambda-function-name)
                deepest_leaf.lock() != nullptr);
            if (node->depth > deepest_leaf.lock()->depth) {
              deepest_leaf = node;
            }
          } else {
            // follow descendants recursively
            for (const auto &child : node->children) {
              handle(child);
            }
          }
        };

    handle(subtree_root_node);
  }

  TreeMeta::TreeMeta(std::unordered_set<primitives::BlockHash> leaves,
                     const std::shared_ptr<TreeNode> &deepest_leaf,
                     const std::shared_ptr<TreeNode> &last_finalized)
      : leaves{std::move(leaves)},
        deepest_leaf{deepest_leaf},
        last_finalized{last_finalized} {}

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

  TreeNode const &CachedTree::getRoot() const {
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
    BOOST_ASSERT(metadata_->deepest_leaf.lock() != nullptr);
    if (new_node->depth > metadata_->deepest_leaf.lock()->depth) {
      metadata_->deepest_leaf = new_node;
    }
  }

  void CachedTree::removeFromMeta(const std::shared_ptr<TreeNode> &node) {
    auto parent = node->parent.lock();
    if (parent != nullptr) {
      auto it =
          std::find(parent->children.begin(), parent->children.end(), node);
      if (it != parent->children.end()) {
        parent->children.erase(it);
      }
    }

    metadata_->leaves.erase(node->block_hash);
    if ((parent != nullptr) && parent->children.empty()) {
      metadata_->leaves.insert(parent->block_hash);
    }
    if (metadata_->deepest_leaf.expired()) {
      std::vector<primitives::BlockInfo> leaf_depths;
      leaf_depths.reserve(metadata_->leaves.size());
      std::transform(
          metadata_->leaves.begin(),
          metadata_->leaves.end(),
          std::back_inserter(leaf_depths),
          [this](const auto &hash) {
            auto leaf_node = root_->findByHash(hash);
            BOOST_ASSERT(leaf_node  // NOLINT(bugprone-lambda-function-name)
                         != nullptr);
            return primitives::BlockInfo{leaf_node->depth,
                                         leaf_node->block_hash};
          });
      std::sort(
          leaf_depths.begin(),
          leaf_depths.end(),
          [](auto const &p1, auto const &p2) { return p1.number > p2.number; });
      metadata_->deepest_leaf = root_->findByHash(leaf_depths.back().hash);
    }
  }
}  // namespace kagome::blockchain
