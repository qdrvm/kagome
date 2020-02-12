/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_tree_impl.hpp"

#include <algorithm>

#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/common.hpp"
#include "blockchain/impl/persistent_map_util.hpp"
#include "common/visitor.hpp"
#include "crypto/blake2/blake2b.h"
#include "scale/scale.hpp"
#include "storage/leveldb/leveldb_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, BlockTreeImpl::Error, e) {
  using E = kagome::blockchain::BlockTreeImpl::Error;
  switch (e) {
    case E::TARGET_IS_PAST_MAX:
      return "target block number is past the given maximum number";
    case E::BLOCK_ON_DEAD_END:
      return "block resides on a dead fork";
    case E::BLOCK_NOT_FOUND:
      return "block exists in chain but not found when following all leaves "
             "backwards.";
  }
  return "unknown error";
}

namespace kagome::blockchain {
  using Buffer = common::Buffer;
  using Prefix = prefix::Prefix;
  using LevelDBError = kagome::storage::LevelDBError;

  BlockTreeImpl::TreeNode::TreeNode(primitives::BlockHash hash,
                                    primitives::BlockNumber depth,
                                    const std::shared_ptr<TreeNode> &parent,
                                    bool finalized)
      : block_hash{hash}, depth{depth}, parent{parent}, finalized{finalized} {}

  std::shared_ptr<BlockTreeImpl::TreeNode> BlockTreeImpl::TreeNode::getByHash(
      const primitives::BlockHash &hash) {
    // standard BFS
    std::queue<std::shared_ptr<TreeNode>> nodes_to_scan;
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

  bool BlockTreeImpl::TreeNode::operator==(const TreeNode &other) const {
    const auto &other_parent = other.parent;
    auto parents_equal = (parent.expired() && other_parent.expired())
                         || (!parent.expired() && !other_parent.expired()
                             && parent.lock() == other_parent.lock());

    return parents_equal && block_hash == other.block_hash
           && depth == other.depth;
  }

  bool BlockTreeImpl::TreeNode::operator!=(const TreeNode &other) const {
    return !(*this == other);
  }

  BlockTreeImpl::TreeMeta::TreeMeta(
      std::unordered_set<primitives::BlockHash> leaves,
      TreeNode &deepest_leaf,
      TreeNode &last_finalized)
      : leaves{std::move(leaves)},
        deepest_leaf{deepest_leaf},
        last_finalized{last_finalized} {}

  outcome::result<std::shared_ptr<BlockTreeImpl>> BlockTreeImpl::create(
      std::shared_ptr<BlockHeaderRepository> header_repo,
      std::shared_ptr<BlockStorage> storage,
      const primitives::BlockId &last_finalized_block,
      std::shared_ptr<crypto::Hasher> hasher) {
    // retrieve the block's header: we need data from it
    OUTCOME_TRY(header, storage->getBlockHeader(last_finalized_block));
    // create meta structures from the retrieved header
    OUTCOME_TRY(hash_res, header_repo->getHashById(last_finalized_block));

    auto tree =
        std::make_shared<TreeNode>(hash_res, header.number, nullptr, true);
    auto meta = std::make_shared<TreeMeta>(
        decltype(TreeMeta::leaves){tree->block_hash}, *tree, *tree);

    BlockTreeImpl block_tree{std::move(header_repo),
                             std::move(storage),
                             std::move(tree),
                             std::move(meta),
                             std::move(hasher)};
    return std::make_shared<BlockTreeImpl>(std::move(block_tree));
  }

  BlockTreeImpl::BlockTreeImpl(
      std::shared_ptr<BlockHeaderRepository> header_repo,
      std::shared_ptr<BlockStorage> storage,
      std::shared_ptr<TreeNode> tree,
      std::shared_ptr<TreeMeta> meta,
      std::shared_ptr<crypto::Hasher> hasher)
      : header_repo_{std::move(header_repo)},
        storage_{std::move(storage)},
        tree_{std::move(tree)},
        tree_meta_{std::move(meta)},
        hasher_{std::move(hasher)} {}

  outcome::result<void> BlockTreeImpl::addBlock(primitives::Block block) {
    // first of all, check if we know parent of this block; if not, we cannot
    // insert it
    auto parent = tree_->getByHash(block.header.parent_hash);
    if (!parent) {
      return BlockTreeError::NO_PARENT;
    }
    OUTCOME_TRY(block_hash, storage_->putBlock(block));
    // update local meta with the new block
    auto new_node =
        std::make_shared<TreeNode>(block_hash, block.header.number, parent);
    parent->children.push_back(new_node);

    tree_meta_->leaves.insert(new_node->block_hash);
    tree_meta_->leaves.erase(parent->block_hash);
    if (new_node->depth > tree_meta_->deepest_leaf.get().depth) {
      tree_meta_->deepest_leaf = *new_node;
    }

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::finalize(
      const primitives::BlockHash &block,
      const primitives::Justification &justification) {
    auto node = tree_->getByHash(block);
    if (!node) {
      return BlockTreeError::NO_SUCH_BLOCK;
    }

    // insert justification into the database
    OUTCOME_TRY(storage_->putJustification(justification, block, node->depth));

    // update our local meta
    tree_meta_->last_finalized = *node;
    node->finalized = true;
    OUTCOME_TRY(prune());

    return outcome::success();
  }

  outcome::result<primitives::BlockBody> BlockTreeImpl::getBlockBody(
      const primitives::BlockId &block) const {
    return storage_->getBlockBody(block);
  }

  outcome::result<primitives::Justification>
  BlockTreeImpl::getBlockJustification(const primitives::BlockId &block) const {
    return storage_->getJustification(block);
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlock(
      const primitives::BlockHash &block) {
    return getChainByBlocks(tree_meta_->last_finalized.get().block_hash, block);
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlock(
      const primitives::BlockHash &block, bool ascending, uint64_t maximum) {
    auto block_number_res = header_repo_->getNumberByHash(block);
    if (!block_number_res) {
      log_->error("cannot retrieve block with hash {}: {}",
                  block,
                  block_number_res.error().message());
      return BlockTreeError::NO_SUCH_BLOCK;
    }
    auto start_block_number = block_number_res.value();

    primitives::BlockNumber finish_block_number;
    if (ascending) {
      if (start_block_number < maximum) {
        // we want to finish at the root
        finish_block_number = 0;
      } else {
        // some non-root block
        finish_block_number = start_block_number - maximum;
      }
    } else {
      auto finish_block_number_candidate = start_block_number + maximum;
      auto current_depth = tree_meta_->deepest_leaf.get().depth;
      if (finish_block_number_candidate <= current_depth) {
        finish_block_number = finish_block_number_candidate;
      } else {
        finish_block_number = current_depth;
      }
    }

    auto finish_block_hash = header_repo_->getHashByNumber(finish_block_number);
    if (!finish_block_hash) {
      log_->error("cannot retrieve block with number {}: {}",
                  finish_block_number,
                  finish_block_hash.error().message());
      return BlockTreeError::NO_SUCH_BLOCK;
    }

    if (!ascending) {
      return getChainByBlocks(block, finish_block_hash.value());
    }

    // the function returns the blocks in the chronological order, but we want a
    // reverted one in this case
    OUTCOME_TRY(chain, getChainByBlocks(finish_block_hash.value(), block));
    std::reverse(chain.begin(), chain.end());
    return std::move(chain);
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlocks(
      const primitives::BlockHash &top_block,
      const primitives::BlockHash &bottom_block) {
    static constexpr std::string_view kNotAncestorError =
        "impossible to get chain by blocks: most probably, block {} is "
        "not an ancestor of {}";
    std::vector<primitives::BlockHash> result;

    auto top_block_node_ptr = tree_->getByHash(top_block);
    auto bottom_block_node_ptr = tree_->getByHash(bottom_block);

    // if both nodes are in our light tree, we can use this representation only
    if (top_block_node_ptr && bottom_block_node_ptr) {
      auto current_node = bottom_block_node_ptr;
      while (current_node != top_block_node_ptr) {
        result.push_back(current_node->block_hash);
        if (auto parent = current_node->parent; !parent.expired()) {
          current_node = parent.lock();
        } else {
          log_->error(kNotAncestorError.data(), top_block, bottom_block);
          return BlockTreeError::INCORRECT_ARGS;
        }
      }
      result.push_back(top_block_node_ptr->block_hash);
      std::reverse(result.begin(), result.end());
      return result;
    }

    // else, we need to use a database
    auto current_hash = bottom_block;
    while (current_hash != top_block) {
      result.push_back(current_hash);
      auto current_header_res = header_repo_->getBlockHeader(current_hash);
      if (!current_header_res) {
        log_->error(kNotAncestorError.data(), top_block, bottom_block);
        return BlockTreeError::INCORRECT_ARGS;
      }
      current_hash = current_header_res.value().parent_hash;
    }
    result.push_back(current_hash);
    std::reverse(result.begin(), result.end());
    return result;
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::longestPath() {
    auto &&[_, block_hash] = deepestLeaf();
    return getChainByBlock(block_hash);
  }

  primitives::BlockInfo BlockTreeImpl::deepestLeaf() const {
    auto &&leaf = tree_meta_->deepest_leaf.get();
    return {leaf.depth, leaf.block_hash};
  }

  outcome::result<primitives::BlockInfo> BlockTreeImpl::getBestContaining(
      const primitives::BlockHash &target_hash,
      const boost::optional<primitives::BlockNumber> &max_number) const {
    OUTCOME_TRY(target_header, header_repo_->getBlockHeader(target_hash));
    if (max_number.has_value() && target_header.number > max_number.value()) {
      return Error::TARGET_IS_PAST_MAX;
    }
    OUTCOME_TRY(canon_hash,
                header_repo_->getHashByNumber(target_header.number));
    // if a max number is given we try to fetch the block at the
    // given depth, if it doesn't exist or `max_number` is not
    // provided, we continue to search from all leaves below.
    if (canon_hash == target_hash) {
      if (max_number.has_value()) {
        auto header = header_repo_->getBlockHeader(max_number.value());
        if (header) {
          OUTCOME_TRY(hash,
                      header_repo_->getHashByNumber(header.value().number));
          return primitives::BlockInfo{header.value().number, hash};
        }
      }
    } else {
      OUTCOME_TRY(last_finalized,
                  header_repo_->getNumberByHash(getLastFinalized().block_hash));
      if (last_finalized >= target_header.number) {
        return Error::BLOCK_ON_DEAD_END;
      }
    }
    for (auto &leaf_hash : getLeavesSorted()) {
      auto current_hash = leaf_hash;
      auto best_hash = current_hash;
      if (max_number.has_value()) {
        OUTCOME_TRY(hash, walkBackUntilLess(current_hash, max_number.value()));
        best_hash = hash;
        current_hash = hash;
      }
      OUTCOME_TRY(best_header, header_repo_->getBlockHeader(best_hash));
      while (true) {
        OUTCOME_TRY(current_header, header_repo_->getBlockHeader(current_hash));
        if (current_hash == target_hash) {
          return primitives::BlockInfo{best_header.number, best_hash};
        }
        if (current_header.number < target_header.number) {
          break;
        }
        current_hash = current_header.parent_hash;
      }
    }

    log_->warn(
        "Block {:?} exists in chain but not found when following all \
			leaves backwards. Max block number = {:?}",
        target_hash,
        max_number.has_value() ? max_number.value() : -1);
    return Error::BLOCK_NOT_FOUND;
  }

  std::vector<primitives::BlockHash> BlockTreeImpl::getLeaves() const {
    std::vector<primitives::BlockHash> result;
    result.reserve(tree_meta_->leaves.size());
    std::transform(tree_meta_->leaves.begin(),
                   tree_meta_->leaves.end(),
                   std::back_inserter(result),
                   [](const auto &hash) { return hash; });
    return result;
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChildren(
      const primitives::BlockHash &block) {
    auto node = tree_->getByHash(block);
    if (!node) {
      return BlockTreeError::NO_SUCH_BLOCK;
    }

    std::vector<primitives::BlockHash> result;
    result.reserve(node->children.size());
    for (const auto &child : node->children) {
      result.push_back(child->block_hash);
    }

    return result;
  }

  primitives::BlockInfo BlockTreeImpl::getLastFinalized() const {
    auto last = tree_meta_->last_finalized.get();
    return primitives::BlockInfo{last.depth, last.block_hash};
  }

  outcome::result<void> BlockTreeImpl::prune() {
    if (tree_meta_->last_finalized.get().parent.expired()) {
      // nothing to prune
      return outcome::success();
    }

    // remove all non-finalized blocks from both database and meta
    std::vector<std::pair<primitives::BlockHash, primitives::BlockNumber>>
        to_remove;

    auto last_node_hash = tree_meta_->last_finalized.get().block_hash;
    auto current_node = tree_meta_->last_finalized.get().parent.lock();
    do {
      const auto &node_children = current_node->children;

      // memorize the hashes and numbers of block to be removed
      for (const auto &child : node_children) {
        if (child->block_hash != last_node_hash) {
          to_remove.emplace_back(child->block_hash, child->depth);
        }
      }

      // go up to the next node
      last_node_hash = current_node->block_hash;
      if (current_node->parent.expired()) {
        break;
      }
      current_node = current_node->parent.lock();
    } while (!current_node->finalized);

    // leave only the last finalized block in memory, as we don't need anything
    // else
    tree_ = std::make_shared<TreeNode>(tree_meta_->last_finalized);
    tree_->parent.reset();
    tree_meta_ = std::make_shared<TreeMeta>(
        decltype(TreeMeta::leaves){tree_->block_hash}, *tree_, *tree_);

    // now, remove the blocks we remembered from the database
    for (const auto &[hash, number] : to_remove) {
      OUTCOME_TRY(storage_->removeBlock(hash, number));
    }
    return outcome::success();
  }

  std::vector<primitives::BlockHash> BlockTreeImpl::getLeavesSorted() const {
    std::vector<primitives::BlockInfo> leaf_depths;
    auto leaves = getLeaves();
    leaf_depths.reserve(leaves.size());
    for (auto &leaf : leaves) {
      auto leaf_node = tree_->getByHash(leaf);
      leaf_depths.emplace_back(
          primitives::BlockInfo{leaf_node->depth, leaf_node->block_hash});
    }
    std::sort(leaf_depths.begin(),
              leaf_depths.end(),
              [](auto const &p1, auto const &p2) {
                return p1.block_number > p2.block_number;
              });
    std::vector<primitives::BlockHash> leaf_hashes;
    leaf_hashes.reserve(leaf_depths.size());
    std::transform(leaf_depths.begin(),
                   leaf_depths.end(),
                   std::back_inserter(leaf_hashes),
                   [](auto &p) { return p.block_hash; });
    return leaf_hashes;
  }

  outcome::result<primitives::BlockHash> BlockTreeImpl::walkBackUntilLess(
      const primitives::BlockHash &start,
      const primitives::BlockNumber &limit) const {
    auto current_hash = start;
    while (true) {
      OUTCOME_TRY(current_header, header_repo_->getBlockHeader(current_hash));
      if (current_header.number <= limit) {
        return current_hash;
      }
      current_hash = current_header.parent_hash;
    }
  }

}  // namespace kagome::blockchain
