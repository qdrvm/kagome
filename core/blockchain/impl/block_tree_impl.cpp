/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_tree_impl.hpp"

#include <boost/assert.hpp>
#include "blockchain/impl/common.hpp"
#include "blockchain/impl/level_db_util.hpp"
#include "common/visitor.hpp"
#include "crypto/blake2/blake2b.h"
#include "scale/scale.hpp"
#include "storage/leveldb/leveldb_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, BlockTreeImpl::Error, e) {
  using E = kagome::blockchain::BlockTreeImpl::Error;
  switch (e) {
    case E::INVALID_DB:
      return "genesis block is not provided, and the database is either empty "
             "or does not contain valid block tree";
    case E::NO_PARENT:
      return "block, which was tried to be added, has no known parent";
    case E::HASH_FAILED:
      return "attempt to hash block part has failed";
    case E::NO_SUCH_BLOCK:
      return "block with such hash cannot be found in the local storage";
    case E::INTERNAL_ERROR:
      return "internal error happened";
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
      : block_hash_{hash},
        depth_{depth},
        parent_{parent},
        finalized_{finalized} {}

  std::shared_ptr<BlockTreeImpl::TreeNode>
  BlockTreeImpl::TreeNode::getByHash(const primitives::BlockHash &hash) {
    // standard BFS
    std::queue<std::shared_ptr<TreeNode>> nodes_to_scan;
    nodes_to_scan.push(shared_from_this());
    while (!nodes_to_scan.empty()) {
      const auto &node = nodes_to_scan.front();
      if (node->block_hash_ == hash) {
        return node;
      }
      for (const auto &child : node->children_) {
        nodes_to_scan.push(child);
      }
      nodes_to_scan.pop();
    }
    return nullptr;
  }

  bool BlockTreeImpl::TreeNode::operator==(const TreeNode &other) const {
    const auto &other_parent = other.parent_;
    auto parents_equal = (parent_.expired() && other_parent.expired())
        || (!parent_.expired() && !other_parent.expired()
            && parent_.lock() == other_parent.lock());

    return parents_equal && block_hash_ == other.block_hash_
        && depth_ == other.depth_;
  }

  bool BlockTreeImpl::TreeNode::operator!=(const TreeNode &other) const {
    return !(*this == other);
  }

  BlockTreeImpl::TreeMeta::TreeMeta(
      std::unordered_set<primitives::BlockHash> leaves, TreeNode &deepest_leaf,
      TreeNode &last_finalized)
      : leaves{std::move(leaves)},
        deepest_leaf{deepest_leaf},
        last_finalized{last_finalized} {}

  outcome::result<std::unique_ptr<BlockTreeImpl>> BlockTreeImpl::create(
      PersistentBufferMap &db, const primitives::BlockId &last_finalized_block,
      std::shared_ptr<crypto::Hasher> hasher,
      common::Logger log) {
    // retrieve the block's header: we need data from it
    OUTCOME_TRY(encoded_header,
                getWithPrefix(db, Prefix::HEADER, last_finalized_block));
    OUTCOME_TRY(header, scale::decode<primitives::BlockHeader>(encoded_header));

    // create meta structures from the retrieved header
    auto hash_res = visit_in_place(
        last_finalized_block,
        [&db, &last_finalized_block, &header,
         &hasher](const primitives::BlockNumber &)
            -> outcome::result<common::Hash256> {
          // number is not enough for out meta: calculate the hash as well
          OUTCOME_TRY(encoded_body,
                      getWithPrefix(db, Prefix::BODY, last_finalized_block));
          OUTCOME_TRY(body, scale::decode<primitives::BlockBody>(encoded_body));
          OUTCOME_TRY(encoded_block,
                      scale::encode(primitives::Block{header, body}));
          return hasher->blake2b_256(encoded_block);
        },
        [](const common::Hash256 &hash) { return hash; });
    if (!hash_res) {
      return hash_res.error();
    }

    auto tree = std::make_shared<TreeNode>(hash_res.value(), header.number,
                                           nullptr, true);
    auto meta = std::make_shared<TreeMeta>(
        decltype(TreeMeta::leaves){tree->block_hash_}, *tree, *tree);

    BlockTreeImpl block_tree{db, std::move(tree), std::move(meta),
                             std::move(hasher), std::move(log)};
    return std::make_unique<BlockTreeImpl>(std::move(block_tree));
  }

  BlockTreeImpl::BlockTreeImpl(PersistentBufferMap &db,
                               std::shared_ptr<TreeNode> tree,
                               std::shared_ptr<TreeMeta> meta,
                               std::shared_ptr<crypto::Hasher> hasher,
                               common::Logger log)
      : db_{db},
        tree_{std::move(tree)},
        tree_meta_{std::move(meta)},
        hasher_{std::move(hasher)},
        log_{std::move(log)} {}

  outcome::result<primitives::BlockBody> BlockTreeImpl::getBlockBody(
      const primitives::BlockId &block) const {
    auto body_res = getWithPrefix(db_, Prefix::BODY, block);
    if (!body_res) {
      return body_res.error();
    }
    return scale::decode<primitives::BlockBody>(body_res.value());
  }

  outcome::result<void> BlockTreeImpl::addBlock(primitives::Block block) {
    // first of all, check if we know parent of this block; if not, we cannot
    // insert it
    auto parent = tree_->getByHash(block.header.parent_hash);
    if (!parent) {
      return Error::NO_PARENT;
    }

    OUTCOME_TRY(encoded_block, scale::encode(block));
    auto block_hash = hasher_->blake2b_256(encoded_block);

    // insert our block's parts into the database
    OUTCOME_TRY(encoded_header, scale::encode(block.header));
    OUTCOME_TRY(putWithPrefix(db_, Prefix::HEADER, block.header.number,
                              block_hash, Buffer{encoded_header}));

    OUTCOME_TRY(encoded_body, scale::encode(block.body));
    OUTCOME_TRY(putWithPrefix(db_, Prefix::BODY, block.header.number,
                              block_hash, Buffer{encoded_body}));

    // update local meta with the new block
    auto new_node =
        std::make_shared<TreeNode>(block_hash, block.header.number, parent);
    parent->children_.push_back(new_node);

    tree_meta_->leaves.insert(new_node->block_hash_);
    tree_meta_->leaves.erase(parent->block_hash_);
    if (new_node->depth_ > tree_meta_->deepest_leaf.get().depth_) {
      tree_meta_->deepest_leaf = *new_node;
    }

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::finalize(
      const primitives::BlockHash &block,
      const primitives::Justification &justification) {
    auto node = tree_->getByHash(block);
    if (!node) {
      return Error::NO_SUCH_BLOCK;
    }

    // insert justification into the database
    OUTCOME_TRY(encoded_justification, scale::encode(justification));
    OUTCOME_TRY(putWithPrefix(db_, Prefix::JUSTIFICATION, node->depth_, block,
                              Buffer{encoded_justification}));

    // update our local meta
    tree_meta_->last_finalized = *node;
    node->finalized_ = true;
    prune();

    return outcome::success();
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlock(
      const primitives::BlockHash &block) {
    auto node = tree_->getByHash(block);
    if (!node) {
      return Error::NO_SUCH_BLOCK;
    }

    std::vector<primitives::BlockHash> result;
    result.push_back(node->block_hash_);
    while (*node != tree_meta_->last_finalized) {
      if (node->parent_.expired()) {
        // should not be here: any node in our tree must be a descendant of the
        // last finalized block
        return Error::INTERNAL_ERROR;
      }
      node = node->parent_.lock();
      result.push_back(node->block_hash_);
    }

    return result;
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::longestPath() {
    return getChainByBlock(deepestLeaf());
  }

  const primitives::BlockHash &BlockTreeImpl::deepestLeaf() const {
    return tree_meta_->deepest_leaf.get().block_hash_;
  }

  std::vector<primitives::BlockHash> BlockTreeImpl::getLeaves() const {
    std::vector<primitives::BlockHash> result;
    result.reserve(tree_meta_->leaves.size());
    std::transform(tree_meta_->leaves.begin(), tree_meta_->leaves.end(),
                   std::back_inserter(result),
                   [](const auto &hash) { return hash; });
    return result;
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChildren(
      const primitives::BlockHash &block) {
    auto node = tree_->getByHash(block);
    if (!node) {
      return Error::NO_SUCH_BLOCK;
    }

    std::vector<primitives::BlockHash> result;
    result.reserve(node->children_.size());
    for (const auto &child : node->children_) {
      result.push_back(child->block_hash_);
    }

    return result;
  }

  primitives::BlockHash BlockTreeImpl::getLastFinalized() const {
    return tree_meta_->last_finalized.get().block_hash_;
  }

  void BlockTreeImpl::prune() {
    if (tree_meta_->last_finalized.get().parent_.expired()) {
      // nothing to prune
      return;
    }

    // remove all non-finalized blocks from both database and meta
    std::vector<std::pair<primitives::BlockHash, primitives::BlockNumber>>
        to_remove;

    auto last_node_hash = tree_meta_->last_finalized.get().block_hash_;
    auto current_node = tree_meta_->last_finalized.get().parent_.lock();
    do {
      const auto &node_children = current_node->children_;

      // memorize the hashes and numbers of block to be removed
      for (const auto &child : node_children) {
        if (child->block_hash_ != last_node_hash) {
          to_remove.emplace_back(child->block_hash_, child->depth_);
        }
      }

      // go up to the next node
      last_node_hash = current_node->block_hash_;
      if (current_node->parent_.expired()) {
        break;
      }
      current_node = current_node->parent_.lock();
    } while (!current_node->finalized_);

    // leave only the last finalized block in memory, as we don't need anything
    // else
    tree_ = std::make_shared<TreeNode>(tree_meta_->last_finalized);
    tree_->parent_.reset();
    tree_meta_ = std::make_shared<TreeMeta>(
        decltype(TreeMeta::leaves){tree_->block_hash_}, *tree_, *tree_);

    // now, remove the blocks we remembered from the database
    for (const auto &[hash, number] : to_remove) {
      auto block_lookup_key = numberAndHashToLookupKey(number, hash);

      auto header_lookup_key = prependPrefix(block_lookup_key, Prefix::HEADER);
      if (auto rm_res = db_.remove(header_lookup_key); !rm_res) {
        log_->error("could not remove header from the storage: {}",
                    rm_res.error().message());
      }

      auto body_lookup_key = prependPrefix(block_lookup_key, Prefix::BODY);
      if (auto rm_res = db_.remove(body_lookup_key); !rm_res) {
        log_->error("could not remove body from the storage: {}",
                    rm_res.error().message());
      }
    }
  }

}  // namespace kagome::blockchain
