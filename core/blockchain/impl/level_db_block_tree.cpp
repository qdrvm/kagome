/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/level_db_block_tree.hpp"

#include <boost/assert.hpp>
#include "blockchain/impl/common.hpp"
#include "blockchain/impl/level_db_util.hpp"
#include "crypto/blake2/blake2b.h"
#include "scale/scale.hpp"
#include "storage/leveldb/leveldb_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, LevelDbBlockTree::Error, e) {
  using E = kagome::blockchain::LevelDbBlockTree::Error;
  switch (e) {
    case E::INVALID_DB:
      return "genesis block is not provided, and the database is either empty "
             "or does not contain valid block tree";
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

  outcome::result<std::reference_wrapper<LevelDbBlockTree::TreeNode>>
  LevelDbBlockTree::TreeNode::getByHash(
      const primitives::BlockHash &hash) const {
    // standard BFS
    std::queue<TreeNode &> nodes_to_scan;
    nodes_to_scan.push(*this);
    while (!nodes_to_scan.empty()) {
      auto &node = nodes_to_scan.front();
      if (node.block_hash == hash) {
        return std::ref(node);
      }
      for (auto &child : node.children) {
        nodes_to_scan.push(child);
      }
      nodes_to_scan.pop();
    }
    return Error::NO_SUCH_BLOCK;
  }

  //  outcome::result<std::unique_ptr<LevelDbBlockTree>>
  //  LevelDbBlockTree::create(
  //      PersistentBufferMap &db,
  //      boost::optional<primitives::Block> genesis_block) {
  //    if (!genesis_block && !initializeMeta()) {
  //    }
  //  }

  //  LevelDbBlockTree::LevelDbBlockTree(
  //      PersistentBufferMap &db, boost::optional<primitives::Block>
  //      genesis_block) : db_{db} {}

  outcome::result<primitives::BlockBody> LevelDbBlockTree::getBlockBody(
      const primitives::BlockId &block) const {
    auto body_res = getWithPrefix(db_, Prefix::BODY, block);
    if (!body_res) {
      return (body_res.error() == LevelDBError::NOT_FOUND)
          ? LevelDBError::NOT_FOUND
          : body_res.error();
    }
    return scale::decode<primitives::BlockBody>(body_res.value());
  }

  outcome::result<void> LevelDbBlockTree::addBlock(primitives::Block block) {
    // first of all, check if we know parent of this block; if not, we cannot
    // insert it
    OUTCOME_TRY(parent_wrapped, tree_.getByHash(block.header.parent_hash));

    OUTCOME_TRY(encoded_block, scale::encode(block));
    common::Hash256 block_hash{};
    if (blake2b(block_hash.data(), common::Hash256::size(), nullptr, 0,
                encoded_block.data(), encoded_block.size())) {
      return Error::HASH_FAILED;
    }

    // insert our block's parts into the database
    OUTCOME_TRY(encoded_header, scale::encode(block.header));
    OUTCOME_TRY(putWithPrefix(db_, Prefix::HEADER, block.header.number,
                              block_hash, Buffer{encoded_header}));

    OUTCOME_TRY(encoded_body, scale::encode(block.extrinsics));
    OUTCOME_TRY(putWithPrefix(db_, Prefix::HEADER, block.header.number,
                              block_hash, Buffer{encoded_body}));

    // update local meta with the new block
    auto &parent = parent_wrapped.get();
    auto &new_node = *parent.children.insert(
        parent.children.end(),
        TreeNode{block_hash, block.header.number, parent});

    tree_meta_.leaves.insert(new_node);
    tree_meta_.leaves.erase(parent);
    if (new_node.depth > tree_meta_.deepest_leaf.depth) {
      tree_meta_.deepest_leaf = new_node;
    }
  }

  outcome::result<void> LevelDbBlockTree::finalizeBlock(
      const primitives::BlockHash &block,
      const primitives::Justification &justification) {
    OUTCOME_TRY(node_wrapped, tree_.getByHash(block));
    auto &node = node_wrapped.get();

    // insert justification into the database
    OUTCOME_TRY(encoded_justification, scale::encode(justification));
    OUTCOME_TRY(putWithPrefix(db_, Prefix::JUSTIFICATION, node.depth, block,
                              Buffer{encoded_justification}));

    // update our local meta
    tree_meta_.last_finalized = node;
    node.finalized = true;
    prune();
  }

  LevelDbBlockTree::BlockHashVecRes LevelDbBlockTree::getChainByBlock(
      const primitives::BlockHash &block) const {
    OUTCOME_TRY(node, tree_.getByHash(block));

    std::vector<primitives::BlockHash> result;
    result.push_back(node.get().block_hash);
    while (node.get() != tree_meta_.last_finalized) {
      if (!node.get().parent) {
        // should not be here: any node in our tree must be a descendant of the
        // last finalized block
        return Error::INTERNAL_ERROR;
      }
      node = *node.get().parent;
    }

    return result;
  }

  LevelDbBlockTree::BlockHashVecRes LevelDbBlockTree::longestPath(
      const primitives::BlockHash &block) const {
    return getChainByBlock(deepestLeaf());
  }

  const primitives::BlockHash &LevelDbBlockTree::deepestLeaf() const {
    return tree_meta_.deepest_leaf;
  }

  std::vector<primitives::BlockHash> LevelDbBlockTree::getLeaves() const {
    std::vector<primitives::BlockHash> result;
    result.reserve(tree_meta_.leaves.size());
    std::copy(tree_meta_.leaves.begin(), tree_meta_.leaves.end(),
              result.begin());
    return result;
  }

  LevelDbBlockTree::BlockHashVecRes LevelDbBlockTree::getChildren(
      const primitives::BlockHash &block) const {
    OUTCOME_TRY(node_wrapped, tree_.getByHash(block));
    auto &node = node_wrapped.get();

    std::vector<primitives::BlockHash> result;
    result.reserve(node.children.size());
    for (const auto &child : node.children) {
      result.push_back(child.block_hash);
    }

    return result;
  }

  primitives::BlockHash LevelDbBlockTree::getLastFinalized() const {
    return tree_meta_.last_finalized.block_hash;
  }

  void LevelDbBlockTree::prune() {
    // remove all non-finalized blocks from both database and meta
    std::vector<std::pair<primitives::BlockHash, primitives::BlockNumber>>
        to_remove;

    auto last_node_hash = tree_meta_.last_finalized.block_hash;
    auto current_node = std::ref(*tree_meta_.last_finalized.parent);
    do {
      // memorize the hashes and numbers of block to be removed
      for (const auto &child : current_node.get().children) {
        if (child.block_hash != last_node_hash) {
          to_remove.push_back({child.block_hash, child.depth});
        }
      }
      // remove them from the meta
      std::remove_if(current_node.get().children.begin(),
                     current_node.get().children.end(),
                     [&last_node_hash](const auto &node) {
                       return node.block_hash != last_node_hash;
                     });
      // go up to the next node
      if (auto parent = current_node.get().parent) {
        current_node = *parent;
      }
      last_node_hash = current_node.get().block_hash;
    } while (!current_node.get().finalized);

    // now, remove the blocks we remembered from the database
    for (const auto &[hash, number] : to_remove) {
      auto block_lookup_key = numberAndHashToLookupKey(number, hash);

      auto header_lookup_key = prependPrefix(block_lookup_key, Prefix::HEADER);
      (void)db_.remove(header_lookup_key);  // not really interested

      auto body_lookup_key = prependPrefix(block_lookup_key, Prefix::BODY);
      (void)db_.remove(body_lookup_key);
    }
  }

}  // namespace kagome::blockchain
