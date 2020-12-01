/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_tree_impl.hpp"

#include <algorithm>

#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/common.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "common/visitor.hpp"
#include "crypto/blake2/blake2b.h"
#include "scale/scale.hpp"
#include "storage/database_error.hpp"

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
  using DatabaseError = kagome::storage::DatabaseError;

  BlockTreeImpl::TreeNode::TreeNode(primitives::BlockHash hash,
                                    primitives::BlockNumber depth,
                                    const std::shared_ptr<TreeNode> &parent,
                                    bool finalized)
      : block_hash{hash}, depth{depth}, parent{parent}, finalized{finalized} {}

  boost::optional<std::vector<std::shared_ptr<BlockTreeImpl::TreeNode>>>
  BlockTreeImpl::TreeNode::getPathTo(const primitives::BlockHash &hash) {
    std::vector<std::shared_ptr<TreeNode>> stack;
    std::vector<std::shared_ptr<TreeNode>> to_check;
    to_check.emplace_back(shared_from_this());

    do {
      auto target = to_check.back();
      to_check.pop_back();

      if (target->block_hash == hash) {
        stack.emplace_back(target);
        return stack;
      }

      auto parent = target->parent.lock();
      while (!stack.empty() && parent != stack.back()) {
        stack.pop_back();
      }

      stack.emplace_back(target);
      std::copy(target->children.begin(),
                target->children.end(),
                std::back_inserter(to_check));
    } while (!to_check.empty());
    return boost::none;
  }

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

  BlockTreeImpl::TreeMeta::TreeMeta(TreeNode &subtree_root_node)
      : deepest_leaf{subtree_root_node}, last_finalized{subtree_root_node} {
    std::function<void(std::shared_ptr<TreeNode>)> handle =
        [&](std::shared_ptr<TreeNode> node) {
          // avoid of deep recurse
          while (node->children.size() == 1) {
            node = node->children.front();
          }

          // is leaf
          if (node->children.empty()) {
            leaves.emplace(node->block_hash);

            if (node->depth > deepest_leaf.get().depth) {
              deepest_leaf = *node;
            }
          } else {
            // follow descendants recursively
            for (const auto &child : node->children) {
              handle(child);
            }
          }
        };

    handle(subtree_root_node.shared_from_this());
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
      std::shared_ptr<network::ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<crypto::Hasher> hasher,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      primitives::events::ExtrinsicSubscriptionEnginePtr
          extrinsic_events_engine,
      std::shared_ptr<runtime::Core> runtime_core) {
    // retrieve the block's header: we need data from it
    OUTCOME_TRY(header, storage->getBlockHeader(last_finalized_block));
    // create meta structures from the retrieved header
    OUTCOME_TRY(hash_res, header_repo->getHashById(last_finalized_block));

    auto tree =
        std::make_shared<TreeNode>(hash_res, header.number, nullptr, true);
    auto meta = std::make_shared<TreeMeta>(*tree);

    BlockTreeImpl block_tree{std::move(header_repo),
                             std::move(storage),
                             std::move(tree),
                             std::move(meta),
                             std::move(extrinsic_observer),
                             std::move(hasher),
                             std::move(chain_events_engine),
                             std::move(extrinsic_events_engine),
                             std::move(runtime_core)};
    return std::make_shared<BlockTreeImpl>(std::move(block_tree));
  }

  BlockTreeImpl::BlockTreeImpl(
      std::shared_ptr<BlockHeaderRepository> header_repo,
      std::shared_ptr<BlockStorage> storage,
      std::shared_ptr<TreeNode> tree,
      std::shared_ptr<TreeMeta> meta,
      std::shared_ptr<network::ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<crypto::Hasher> hasher,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      primitives::events::ExtrinsicSubscriptionEnginePtr
          extrinsic_events_engine,
      std::shared_ptr<runtime::Core> runtime_core)
      : header_repo_{std::move(header_repo)},
        storage_{std::move(storage)},
        tree_{std::move(tree)},
        tree_meta_{std::move(meta)},
        extrinsic_observer_{std::move(extrinsic_observer)},
        hasher_{std::move(hasher)},
        chain_events_engine_(std::move(chain_events_engine)),
        extrinsic_events_engine_(std::move(extrinsic_events_engine)),
        runtime_core_(std::move(runtime_core)) {
    BOOST_ASSERT(chain_events_engine_);
    BOOST_ASSERT(extrinsic_events_engine_);
    BOOST_ASSERT(runtime_core_);
  }

  outcome::result<void> BlockTreeImpl::addBlockHeader(
      const primitives::BlockHeader &header) {
    auto parent = tree_->getByHash(header.parent_hash);
    if (!parent) {
      return BlockTreeError::NO_PARENT;
    }
    OUTCOME_TRY(block_hash, storage_->putBlockHeader(header));
    // update local meta with the new block
    auto new_node =
        std::make_shared<TreeNode>(block_hash, header.number, parent);
    parent->children.push_back(new_node);

    tree_meta_->leaves.insert(new_node->block_hash);
    tree_meta_->leaves.erase(parent->block_hash);
    if (new_node->depth > tree_meta_->deepest_leaf.get().depth) {
      tree_meta_->deepest_leaf = *new_node;
    }

    chain_events_engine_->notify(primitives::events::ChainEventType::kNewHeads,
                                 header);

    return outcome::success();
  }

  void BlockTreeImpl::updateMeta(const std::shared_ptr<TreeNode> &new_node) {
    auto parent = new_node->parent.lock();
    parent->children.push_back(new_node);

    tree_meta_->leaves.insert(new_node->block_hash);
    tree_meta_->leaves.erase(parent->block_hash);
    if (new_node->depth > tree_meta_->deepest_leaf.get().depth) {
      tree_meta_->deepest_leaf = *new_node;
    }
  }

  outcome::result<void> BlockTreeImpl::addBlock(
      const primitives::Block &block) {
    // Check if we know parent of this block; if not, we cannot insert it
    auto parent = tree_->getByHash(block.header.parent_hash);
    if (!parent) {
      return BlockTreeError::NO_PARENT;
    }

    // Save block
    OUTCOME_TRY(block_hash, storage_->putBlock(block));

    // Update local meta with the block
    auto new_node =
        std::make_shared<TreeNode>(block_hash, block.header.number, parent);

    updateMeta(new_node);
    chain_events_engine_->notify(primitives::events::ChainEventType::kNewHeads,
                                 block.header);
    for (const auto &ext : block.body) {
      if (ext.observed_id) {
        extrinsic_events_engine_->notify(
            ext.observed_id.value(),
            primitives::events::ExtrinsicLifecycleEvent::InBlock(ext.observed_id.value(), std::move(block_hash)));
      }
    }

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::addExistingBlock(
      const primitives::BlockHash &block_hash,
      const primitives::BlockHeader &block_header) {
    auto node = tree_->getByHash(block_hash);
    // Check if tree doesn't have this block; if not, we skip that
    if (node != nullptr) {
      return BlockTreeError::BLOCK_EXISTS;
    }
    // Check if we know parent of this block; if not, we cannot insert it
    auto parent = tree_->getByHash(block_header.parent_hash);
    if (parent == nullptr) {
      return BlockTreeError::NO_PARENT;
    }

    // Update local meta with the block
    auto new_node =
        std::make_shared<TreeNode>(block_hash, block_header.number, parent);

    updateMeta(new_node);

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::addBlockBody(
      primitives::BlockNumber block_number,
      const primitives::BlockHash &block_hash,
      const primitives::BlockBody &body) {
    primitives::BlockData block_data{.hash = block_hash, .body = body};
    return storage_->putBlockData(block_number, block_data);
  }

  outcome::result<void> BlockTreeImpl::finalize(
      const primitives::BlockHash &block_hash,
      const primitives::Justification &justification) {
    auto node = tree_->getByHash(block_hash);
    if (!node) {
      return BlockTreeError::NO_SUCH_BLOCK;
    }
    if (storage_->getJustification(block_hash)) {
      // block was already finalized, fine
      return outcome::success();
    }

    // insert justification into the database
    OUTCOME_TRY(
        storage_->putJustification(justification, block_hash, node->depth));

    // update our local meta
    node->finalized = true;

    OUTCOME_TRY(prune(node));

    tree_ = node;

    tree_meta_ = std::make_shared<TreeMeta>(*tree_);

    tree_->parent.reset();

    OUTCOME_TRY(storage_->setLastFinalizedBlockHash(node->block_hash));
    OUTCOME_TRY(header, storage_->getBlockHeader(node->block_hash));

    chain_events_engine_->notify(
        primitives::events::ChainEventType::kFinalizedHeads, header);

    OUTCOME_TRY(new_runtime_version, runtime_core_->version(boost::none));
    if (not actual_runtime_version_.has_value()
        || actual_runtime_version_ != new_runtime_version) {
      actual_runtime_version_ = new_runtime_version;
      chain_events_engine_->notify(
          primitives::events::ChainEventType::kRuntimeVersion,
          new_runtime_version);
    }
    OUTCOME_TRY(body, storage_->getBlockBody(node->block_hash));

    for (const auto &ext : body) {
      if (ext.observed_id) {
        extrinsic_events_engine_->notify(
            ext.observed_id.value(),
            primitives::events::ExtrinsicLifecycleEvent::Finalized(ext.observed_id.value(), std::move(block_hash)));
      }
    }

    log_->info("Finalized block number {} with hash {}",
               node->depth,
               block_hash.toHex());
    return outcome::success();
  }

  outcome::result<primitives::BlockHeader> BlockTreeImpl::getBlockHeader(
      const primitives::BlockId &block) const {
    return storage_->getBlockHeader(block);
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
                  block.toHex(),
                  block_number_res.error().message());
      return BlockTreeError::NO_SUCH_BLOCK;
    }
    auto start_block_number = block_number_res.value();

    primitives::BlockNumber finish_block_number;  // NOLINT
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
      const primitives::BlockHash &bottom_block,
      const uint32_t max_count) {
    return getChainByBlocks(
        top_block, bottom_block, boost::make_optional(max_count));
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlocks(
      const primitives::BlockHash &top_block,
      const primitives::BlockHash &bottom_block,
      boost::optional<uint32_t> max_count) {
    if (auto from_cache =
            tryGetChainByBlocksFromCache(top_block, bottom_block, max_count)) {
      return std::move(from_cache.value());
    }

    OUTCOME_TRY(from, header_repo_->getNumberByHash(top_block));
    OUTCOME_TRY(to, header_repo_->getNumberByHash(bottom_block));

    std::vector<primitives::BlockHash> result;
    if (to < from) return result;

    const uint64_t response_length =
        max_count ? std::min(to - from + 1, static_cast<uint64_t>(*max_count))
                  : to - from + 1;
    log_->trace("Try to create {} length chain from number {} to {}.",
                response_length,
                from,
                to);

    result.reserve(response_length);
    result.emplace_back(top_block);

    const auto end = from + response_length;
    auto ix = from + 1;
    while (ix < end) {
      OUTCOME_TRY(hash, header_repo_->getHashByNumber(ix));
      result.emplace_back(hash);
      ++ix;
    }
    return result;
  }

  boost::optional<std::vector<primitives::BlockHash>>
  BlockTreeImpl::tryGetChainByBlocksFromCache(
      const primitives::BlockHash &top_block,
      const primitives::BlockHash &bottom_block,
      boost::optional<uint32_t> max_count) {
    if (auto from = tree_->getByHash(top_block)) {
      if (auto way = from->getPathTo(bottom_block)) {
        const uint64_t in_tree_branch_len =
            way->back()->depth - from->depth + 1;
        const uint64_t response_length =
            max_count ? std::min(in_tree_branch_len,
                                 static_cast<uint64_t>(*max_count))
                      : in_tree_branch_len;
        log_->trace("Create {} length chain from number {} to {} from cache.",
                    response_length,
                    from->depth,
                    way->back()->depth);

        way->resize(response_length);

        std::vector<primitives::BlockHash> result;
        result.reserve(response_length);
        for (auto &s : *way) result.emplace_back(s->block_hash);

        return result;
      }
    }
    return boost::none;
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlocks(
      const primitives::BlockHash &top_block,
      const primitives::BlockHash &bottom_block) {
    return getChainByBlocks(top_block, bottom_block, boost::none);
  }

  bool BlockTreeImpl::hasDirectChain(const primitives::BlockHash &ancestor,
                                     const primitives::BlockHash &descendant) {
    auto ancestor_node_ptr = tree_->getByHash(ancestor);
    auto descendant_node_ptr = tree_->getByHash(descendant);

    // if both nodes are in our light tree, we can use this representation only
    if (ancestor_node_ptr && descendant_node_ptr) {
      auto current_node = descendant_node_ptr;
      while (current_node != ancestor_node_ptr) {
        if (auto parent = current_node->parent; !parent.expired()) {
          current_node = parent.lock();
        } else {
          return false;
        }
      }
      return true;
    }

    // else, we need to use a database
    auto current_hash = descendant;
    while (current_hash != ancestor) {
      auto current_header_res = header_repo_->getBlockHeader(current_hash);
      if (!current_header_res) {
        return false;
      }
      current_hash = current_header_res.value().parent_hash;
    }
    return true;
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
        "Block {} exists in chain but not found when following all leaves "
        "backwards. Max block number = {}",
        target_hash.toHex(),
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
    const auto &last = tree_meta_->last_finalized.get();
    return primitives::BlockInfo{last.depth, last.block_hash};
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

  outcome::result<void> BlockTreeImpl::prune(
      const std::shared_ptr<TreeNode> &lastFinalizedNode) {
    std::vector<std::pair<primitives::BlockHash, primitives::BlockNumber>>
        to_remove;

    auto current_node = lastFinalizedNode;

    for (;;) {
      auto parent_node = current_node->parent.lock();
      if (!parent_node || parent_node->finalized) {
        break;
      }

      auto main_chain_node = current_node;
      current_node = parent_node;

      // collect hashes for removing (except main chain block)
      for (const auto &child : current_node->children) {
        if (child->block_hash != main_chain_node->block_hash) {
          collectDescendants(child, to_remove);
          to_remove.emplace_back(child->block_hash, child->depth);
        }
      }

      // remove (in memory) all child, except main chain block
      current_node->children = {main_chain_node};
    }

    std::vector<primitives::Extrinsic> extrinsics;

    // remove from storage
    for (const auto &[hash, number] : to_remove) {
      auto block_body_res = storage_->getBlockBody(hash);
      if (block_body_res.has_value()) {
        extrinsics.reserve(extrinsics.size() + block_body_res.value().size());
        for (auto &&extrinsic : block_body_res.value()) {
          if (extrinsic.observed_id) {
            extrinsic_events_engine_->notify(
                extrinsic.observed_id.value(),
                primitives::events::ExtrinsicLifecycleEvent::Retracted(extrinsic.observed_id.value(), std::move(hash)));
          }
          extrinsics.emplace_back(std::move(extrinsic));
        }
      }

      OUTCOME_TRY(storage_->removeBlock(hash, number));
    }

    // trying to return back extrinsics to transaction pool
    for (auto &&extrinsic : extrinsics) {
      auto result = extrinsic_observer_->onTxMessage(extrinsic);
      if (result) {
        log_->debug("Tx {} was reapplied", result.value().toHex());
      } else {
        log_->debug("Tx was skipped: {}", result.error().message());
      }
    }

    return outcome::success();
  }

  void BlockTreeImpl::collectDescendants(
      std::shared_ptr<TreeNode> node,
      std::vector<std::pair<primitives::BlockHash, primitives::BlockNumber>>
          &container) {
    // avoid deep recursion
    while (node->children.size() == 1) {
      container.emplace_back(node->block_hash, node->depth);
      node = node->children.front();
    }

    // collect descendants' hashes recursively
    for (const auto &child : node->children) {
      collectDescendants(child, container);
      container.emplace_back(child->block_hash, child->depth);
    }
  }
}  // namespace kagome::blockchain
