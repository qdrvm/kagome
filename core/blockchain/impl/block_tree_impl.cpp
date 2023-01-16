/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_tree_impl.hpp"

#include <algorithm>
#include <stack>

#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/cached_tree.hpp"
#include "blockchain/impl/common.hpp"
#include "blockchain/impl/justification_storage_policy.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/is_primary.hpp"
#include "crypto/blake2/blake2b.h"
#include "log/profiling_logger.hpp"
#include "storage/changes_trie/changes_tracker.hpp"
#include "storage/database_error.hpp"

namespace {
  constexpr auto blockHeightMetricName = "kagome_block_height";
  constexpr auto knownChainLeavesMetricName = "kagome_number_leaves";
}  // namespace

namespace kagome::blockchain {
  using Buffer = common::Buffer;
  using Prefix = prefix::Prefix;
  using DatabaseError = kagome::storage::DatabaseError;
  using consensus::babe::isPrimary;

  namespace {
    /// Function-helper for loading (and repair if it needed) of leaves
    outcome::result<std::set<primitives::BlockInfo>> loadLeaves(
        const std::shared_ptr<BlockStorage> &storage,
        const std::shared_ptr<BlockHeaderRepository> &header_repo,
        const log::Logger &log) {
      BOOST_ASSERT(storage != nullptr);
      BOOST_ASSERT(header_repo != nullptr);

      std::set<primitives::BlockInfo> block_tree_leaves;
      {
        OUTCOME_TRY(block_tree_unordered_leaves, storage->getBlockTreeLeaves());
        SL_TRACE(log,
                 "List of leaves has loaded: {} leaves",
                 block_tree_unordered_leaves.size());

        for (auto &hash : block_tree_unordered_leaves) {
          auto res = header_repo->getNumberById(hash);
          if (res.has_error()) {
            if (res
                == outcome::failure(
                    blockchain::BlockTreeError::HEADER_NOT_FOUND)) {
              SL_TRACE(log, "Leaf {} not found", hash);
              continue;
            }
            SL_ERROR(log, "Leaf {} is corrupted: {}", hash, res.error());
            return res.as_failure();
          }
          auto number = res.value();
          SL_TRACE(log, "Leaf {} found", primitives::BlockInfo(number, hash));
          block_tree_leaves.emplace(number, hash);
        }
      }

      if (block_tree_leaves.empty()) {
        SL_WARN(log, "No one leaf was found. Trying to repair");

        primitives::BlockNumber number = 0;
        auto lower = std::numeric_limits<primitives::BlockNumber>::min();
        auto upper = std::numeric_limits<primitives::BlockNumber>::max();

        for (;;) {
          number = lower + (upper - lower) / 2 + 1;

          auto res = storage->hasBlockHeader(number);
          if (res.has_failure()) {
            SL_CRITICAL(log, "Search best block has failed: {}", res.error());
            return BlockTreeError::HEADER_NOT_FOUND;
          }

          if (res.value()) {
            SL_TRACE(log, "bisect {} -> found", number);
            lower = number;
          } else {
            SL_TRACE(log, "bisect {} -> not found", number);
            upper = number - 1;
          }
          if (lower == upper) {
            number = lower;
            break;
          }
        }

        OUTCOME_TRY(hash, header_repo->getHashById(number));
        block_tree_leaves.emplace(number, hash);

        if (auto res = storage->setBlockTreeLeaves({hash}); res.has_error()) {
          SL_CRITICAL(
              log, "Can't save recovered block tree leaves: {}", res.error());
          return res.as_failure();
        }
      }

      return block_tree_leaves;
    }
  }  // namespace

  outcome::result<std::shared_ptr<BlockTreeImpl>> BlockTreeImpl::create(
      std::shared_ptr<BlockHeaderRepository> header_repo,
      std::shared_ptr<BlockStorage> storage,
      std::shared_ptr<network::ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<crypto::Hasher> hasher,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      primitives::events::ExtrinsicSubscriptionEnginePtr
          extrinsic_events_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          extrinsic_event_key_repo,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<const class JustificationStoragePolicy>
          justification_storage_policy) {
    BOOST_ASSERT(storage != nullptr);
    BOOST_ASSERT(header_repo != nullptr);

    log::Logger log = log::createLogger("BlockTree", "block_tree");

    OUTCOME_TRY(block_tree_leaves, loadLeaves(storage, header_repo, log));

    BOOST_ASSERT_MSG(not block_tree_leaves.empty(),
                     "Must be known or calculated at least one leaf");

    // Find the least and best leaf
    auto best_leaf = *block_tree_leaves.rbegin();

    OUTCOME_TRY(last_finalized_block_info, storage->getLastFinalized());

    // call chain_events_engine->notify to init babe_config_repo preventive
    auto finalized_block_header_res =
        storage->getBlockHeader(last_finalized_block_info.hash);
    BOOST_ASSERT_MSG(finalized_block_header_res.has_value()
                         and finalized_block_header_res.value().has_value(),
                     "Initialized block tree must be have finalized block");
    auto &finalized_block_header = finalized_block_header_res.value().value();
    chain_events_engine->notify(
        primitives::events::ChainEventType::kFinalizedHeads,
        finalized_block_header);

    OUTCOME_TRY(last_finalized_justification,
                storage->getJustification(last_finalized_block_info.hash));
    SL_INFO(log,
            "Best block: {}, Last finalized: {}",
            best_leaf,
            last_finalized_block_info);

    // Load non-finalized block from block storage
    std::multimap<primitives::BlockInfo, primitives::BlockHeader> collected;

    {
      std::unordered_set<primitives::BlockHash> observed;
      for (auto &leaf : block_tree_leaves) {
        for (auto hash = leaf.hash;;) {
          if (hash == last_finalized_block_info.hash) {
            break;
          }

          if (not observed.emplace(hash).second) {
            break;
          }

          auto header_res = storage->getBlockHeader(hash);
          if (header_res.has_error()) {
            SL_WARN(log,
                    "Can't get header of existing non-finalized block {}: {}",
                    hash,
                    header_res.error());
            break;
          }
          auto &header_opt = header_res.value();
          if (!header_opt.has_value()) {
            SL_WARN(log,
                    "Can't get header of existing block {}: "
                    "not found in block storage",
                    hash,
                    header_res.error());
            break;
          }

          const auto &header = header_opt.value();
          primitives::BlockInfo block(header.number, hash);

          collected.emplace(block, header);

          hash = header.parent_hash;
        }
      }
    }

    // Prepare and create block tree basing last finalized block
    auto tree = std::make_shared<TreeNode>(last_finalized_block_info.hash,
                                           last_finalized_block_info.number,
                                           nullptr,
                                           true,
                                           isPrimary(finalized_block_header));
    SL_DEBUG(log, "Last finalized block #{}", tree->depth);
    auto meta = std::make_shared<TreeMeta>(tree, last_finalized_justification);

    std::shared_ptr<BlockTreeImpl> block_tree(
        new BlockTreeImpl(std::move(header_repo),
                          std::move(storage),
                          std::make_unique<CachedTree>(tree, meta),
                          std::move(extrinsic_observer),
                          std::move(hasher),
                          std::move(chain_events_engine),
                          std::move(extrinsic_events_engine),
                          std::move(extrinsic_event_key_repo),
                          std::move(changes_tracker),
                          std::move(justification_storage_policy)));

    // Add non-finalized block to the block tree
    for (auto &e : collected) {
      const auto &block = e.first;
      const auto header = std::move(e.second);

      auto res = block_tree->addExistingBlock(block.hash, header);
      if (res.has_error()) {
        SL_WARN(log,
                "Can't add existing non-finalized block {} to block tree: {}",
                block,
                res.error());
      }
      SL_TRACE(
          log, "Existing non-finalized block {} is added to block tree", block);
    }

    return block_tree;
  }

  outcome::result<void> BlockTreeImpl::recover(
      primitives::BlockId target_block,
      std::shared_ptr<BlockStorage> storage,
      std::shared_ptr<BlockHeaderRepository> header_repo,
      std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<blockchain::BlockTree> block_tree) {
    BOOST_ASSERT(storage != nullptr);
    BOOST_ASSERT(header_repo != nullptr);
    BOOST_ASSERT(trie_storage != nullptr);

    log::Logger log = log::createLogger("BlockTree", "block_tree");

    OUTCOME_TRY(block_tree_leaves, loadLeaves(storage, header_repo, log));

    BOOST_ASSERT_MSG(not block_tree_leaves.empty(),
                     "Must be known or calculated at least one leaf");

    // Check if target block exists
    auto target_block_header_opt_res = storage->getBlockHeader(target_block);
    if (target_block_header_opt_res.has_error()) {
      SL_CRITICAL(log,
                  "Can't get header of target block: {}",
                  target_block_header_opt_res.error());
      return target_block_header_opt_res.as_failure();
    }
    const auto &target_block_header_opt = target_block_header_opt_res.value();
    if (not target_block_header_opt.has_value()) {
      return BlockTreeError::HEADER_NOT_FOUND;
    }

    const auto &target_block_header = target_block_header_opt.value();
    const auto &state_root = target_block_header.state_root;

    // Check if target block has state
    if (auto res = trie_storage->getEphemeralBatchAt(state_root, {});
        res.has_error()) {
      SL_WARN(log, "Can't get state of target block: {}", res.error());
      SL_CRITICAL(
          log,
          "You will need to use `--sync Fast' CLI arg the next time you start");
    }

    for (auto it = block_tree_leaves.rbegin(); it != block_tree_leaves.rend();
         it = block_tree_leaves.rbegin()) {
      auto block = *it;
      if (target_block_header.number >= block.number) {
        break;
      }

      auto header_opt_res = storage->getBlockHeader(block.hash);
      if (header_opt_res.has_error()) {
        SL_CRITICAL(log,
                    "Can't get header of one of removing block: {}",
                    header_opt_res.error());
        return header_opt_res.as_failure();
      }
      const auto &header_opt = header_opt_res.value();
      if (not header_opt.has_value()) {
        return BlockTreeError::HEADER_NOT_FOUND;
      }

      const auto &header = header_opt.value();
      block_tree_leaves.emplace(block.number - 1, header.parent_hash);
      block_tree_leaves.erase(block);

      std::vector<primitives::BlockHash> leaves;
      std::transform(block_tree_leaves.begin(),
                     block_tree_leaves.end(),
                     std::back_inserter(leaves),
                     [](const auto it) { return it.hash; });
      if (auto res = storage->setBlockTreeLeaves(leaves); res.has_error()) {
        SL_CRITICAL(
            log, "Can't save updated block tree leaves: {}", res.error());
        return res.as_failure();
      }

      if (auto res = block_tree->removeLeaf(block.hash); res.has_error()) {
        SL_CRITICAL(log, "Can't remove block {}: {}", block, res.error());
        return res.as_failure();
      }
    }

    return outcome::success();
  }

  BlockTreeImpl::BlockTreeImpl(
      std::shared_ptr<BlockHeaderRepository> header_repo,
      std::shared_ptr<BlockStorage> storage,
      std::unique_ptr<CachedTree> cached_tree,
      std::shared_ptr<network::ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<crypto::Hasher> hasher,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine,
      primitives::events::ExtrinsicSubscriptionEnginePtr
          extrinsic_events_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          extrinsic_event_key_repo,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<const JustificationStoragePolicy>
          justification_storage_policy)
      : header_repo_{std::move(header_repo)},
        storage_{std::move(storage)},
        tree_{std::move(cached_tree)},
        extrinsic_observer_{std::move(extrinsic_observer)},
        hasher_{std::move(hasher)},
        chain_events_engine_(std::move(chain_events_engine)),
        extrinsic_events_engine_(std::move(extrinsic_events_engine)),
        extrinsic_event_key_repo_{std::move(extrinsic_event_key_repo)},
        trie_changes_tracker_(std::move(changes_tracker)),
        justification_storage_policy_{std::move(justification_storage_policy)} {
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(tree_ != nullptr);
    BOOST_ASSERT(extrinsic_observer_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(chain_events_engine_ != nullptr);
    BOOST_ASSERT(extrinsic_events_engine_ != nullptr);
    BOOST_ASSERT(extrinsic_event_key_repo_ != nullptr);
    BOOST_ASSERT(trie_changes_tracker_ != nullptr);
    BOOST_ASSERT(justification_storage_policy_ != nullptr);
    BOOST_ASSERT(telemetry_ != nullptr);

    // Register metrics
    metrics_registry_->registerGaugeFamily(blockHeightMetricName,
                                           "Block height info of the chain");

    metric_best_block_height_ = metrics_registry_->registerGaugeMetric(
        blockHeightMetricName, {{"status", "best"}});
    metric_best_block_height_->set(
        tree_->getMetadata().best_leaf.lock()->depth);

    metric_finalized_block_height_ = metrics_registry_->registerGaugeMetric(
        blockHeightMetricName, {{"status", "finalized"}});
    metric_finalized_block_height_->set(
        tree_->getMetadata().last_finalized.lock()->depth);

    metrics_registry_->registerGaugeFamily(
        knownChainLeavesMetricName, "Number of known chain leaves (aka forks)");

    metric_known_chain_leaves_ =
        metrics_registry_->registerGaugeMetric(knownChainLeavesMetricName);
    metric_known_chain_leaves_->set(tree_->getMetadata().leaves.size());

    telemetry_->setGenesisBlockHash(getGenesisBlockHash());
  }

  const primitives::BlockHash &BlockTreeImpl::getGenesisBlockHash() const {
    if (genesis_block_hash_.has_value()) {
      return genesis_block_hash_.value();
    }

    auto res = header_repo_->getHashByNumber(0);
    BOOST_ASSERT_MSG(res.has_value(),
                     "Block tree must contain at least genesis block");

    const_cast<std::decay_t<decltype(genesis_block_hash_)> &>(
        genesis_block_hash_)
        .emplace(res.value());
    return genesis_block_hash_.value();
  }

  outcome::result<void> BlockTreeImpl::addBlockHeader(
      const primitives::BlockHeader &header) {
    auto parent = tree_->getRoot().findByHash(header.parent_hash);
    if (!parent) {
      return BlockTreeError::NO_PARENT;
    }
    OUTCOME_TRY(block_hash, storage_->putBlockHeader(header));

    // update local meta with the new block
    auto new_node = std::make_shared<TreeNode>(
        block_hash, header.number, parent, false, isPrimary(header));

    tree_->updateMeta(new_node);

    OUTCOME_TRY(reorganize());

    OUTCOME_TRY(
        storage_->setBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
                                      tree_->getMetadata().leaves.end()}));

    metric_known_chain_leaves_->set(tree_->getMetadata().leaves.size());
    metric_best_block_height_->set(
        tree_->getMetadata().best_leaf.lock()->depth);

    chain_events_engine_->notify(primitives::events::ChainEventType::kNewHeads,
                                 header);

    SL_VERBOSE(log_,
               "Block {} has been added into block tree",
               primitives::BlockInfo(header.number, block_hash));

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::addBlock(
      const primitives::Block &block) {
    // Check if we know parent of this block; if not, we cannot insert it
    auto parent = tree_->getRoot().findByHash(block.header.parent_hash);
    if (!parent) {
      return BlockTreeError::NO_PARENT;
    }

    // Save block
    OUTCOME_TRY(block_hash, storage_->putBlock(block));

    // Update local meta with the block
    auto new_node = std::make_shared<TreeNode>(block_hash,
                                               block.header.number,
                                               parent,
                                               false,
                                               isPrimary(block.header));

    tree_->updateMeta(new_node);

    OUTCOME_TRY(reorganize());

    OUTCOME_TRY(
        storage_->setBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
                                      tree_->getMetadata().leaves.end()}));

    chain_events_engine_->notify(primitives::events::ChainEventType::kNewHeads,
                                 block.header);
    trie_changes_tracker_->onBlockAdded(block_hash);
    SL_DEBUG(log_, "Adding block {}", block_hash);
    for (const auto &ext : block.body) {
      auto hash = hasher_->blake2b_256(ext.data);
      SL_DEBUG(log_, "Adding extrinsic with hash {}", hash);
      if (auto key = extrinsic_event_key_repo_->get(hash)) {
        extrinsic_events_engine_->notify(
            key.value(),
            primitives::events::ExtrinsicLifecycleEvent::InBlock(key.value(),
                                                                 block_hash));
      }
    }

    metric_known_chain_leaves_->set(tree_->getMetadata().leaves.size());
    metric_best_block_height_->set(
        tree_->getMetadata().best_leaf.lock()->depth);

    SL_VERBOSE(log_,
               "Block {} has been added into block tree",
               primitives::BlockInfo(block.header.number, block_hash));

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::removeLeaf(
      const primitives::BlockHash &block_hash) {
    // Check if block is leaf
    if (tree_->getMetadata().leaves.count(block_hash) == 0) {
      return BlockTreeError::BLOCK_IS_NOT_LEAF;
    }

    auto node = tree_->getRoot().findByHash(block_hash);
    BOOST_ASSERT_MSG(node != nullptr,
                     "As checked before, block exists as one of leaves");

    if (not node->parent.expired()) {
      // Remove from block tree, ...
      tree_->removeFromMeta(node);

      OUTCOME_TRY(reorganize());

    } else {
      // ... or repair tree by parent of root
      auto _header = header_repo_->getBlockHeader(node->depth - 1);
      BOOST_ASSERT_MSG(_header.has_value(),
                       "Non genesis block must have parent");
      auto &header = _header.value();
      primitives::BlockInfo block{
          node->depth - 1,
          hasher_->blake2b_256(scale::encode(header).value()),
      };
      auto tree = std::make_shared<TreeNode>(
          block.hash, block.number, nullptr, true, isPrimary(header));
      auto meta = std::make_shared<TreeMeta>(tree, std::nullopt);
      tree_ = std::make_unique<CachedTree>(std::move(tree), std::move(meta));
    }

    // Remove from storage
    OUTCOME_TRY(storage_->removeBlock({node->depth, node->block_hash}));

    OUTCOME_TRY(
        storage_->setBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
                                      tree_->getMetadata().leaves.end()}));

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::addExistingBlock(
      const primitives::BlockHash &block_hash,
      const primitives::BlockHeader &block_header) {
    SL_TRACE(log_,
             "Trying to add block {} into block tree",
             primitives::BlockInfo(block_header.number, block_hash));

    auto node = tree_->getRoot().findByHash(block_hash);
    // Check if tree doesn't have this block; if not, we skip that
    if (node != nullptr) {
      SL_TRACE(log_,
               "Block {} exists in block tree",
               primitives::BlockInfo(block_header.number, block_hash));
      return BlockTreeError::BLOCK_EXISTS;
    }

    auto parent = tree_->getRoot().findByHash(block_header.parent_hash);

    // Check if we know parent of this block; if not, we cannot insert it
    if (parent == nullptr) {
      SL_TRACE(log_,
               "Block {} parent of {} has not found in block tree. "
               "Trying to restore missed branch",
               primitives::BlockInfo(block_header.number - 1,
                                     block_header.parent_hash),
               primitives::BlockInfo(block_header.number, block_hash));

      // Trying to restore missed branch
      std::stack<std::pair<primitives::BlockHash, primitives::BlockHeader>>
          to_add;

      for (auto hash = block_header.parent_hash;;) {
        OUTCOME_TRY(header_opt, storage_->getBlockHeader(hash));
        if (not header_opt.has_value()) {
          return BlockTreeError::NO_PARENT;
        }

        auto &header = header_opt.value();
        SL_TRACE(log_,
                 "Block {} has found in storage and enqueued to add",
                 primitives::BlockInfo(header.number, hash));

        to_add.emplace(hash, std::move(header));

        if (tree_->getRoot().findByHash(header.parent_hash) != nullptr) {
          SL_TRACE(log_,
                   "Block {} parent of {} has found in block tree",
                   primitives::BlockInfo(header.number - 1, header.parent_hash),
                   primitives::BlockInfo(header.number, hash));

          break;
        }

        SL_TRACE(log_,
                 "Block {} has not found in block tree. "
                 "Trying to restore from storage",
                 primitives::BlockInfo(header.number - 1, header.parent_hash));

        hash = header.parent_hash;
      }

      while (not to_add.empty()) {
        const auto &[hash, header] = to_add.top();
        OUTCOME_TRY(addExistingBlock(hash, header));
        to_add.pop();
      }

      parent = tree_->getRoot().findByHash(block_header.parent_hash);
      BOOST_ASSERT_MSG(parent != nullptr,
                       "Parent must be restored at this moment");

      SL_TRACE(log_,
               "Trying to add block {} into block tree",
               primitives::BlockInfo(block_header.number, block_hash));
    }

    // Update local meta with the block
    auto new_node = std::make_shared<TreeNode>(block_hash,
                                               block_header.number,
                                               parent,
                                               false,
                                               isPrimary(block_header));

    tree_->updateMeta(new_node);

    OUTCOME_TRY(reorganize());

    OUTCOME_TRY(
        storage_->setBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
                                      tree_->getMetadata().leaves.end()}));

    metric_known_chain_leaves_->set(tree_->getMetadata().leaves.size());
    metric_best_block_height_->set(
        tree_->getMetadata().best_leaf.lock()->depth);

    SL_VERBOSE(log_,
               "Block {} has been restored in block tree from storage",
               primitives::BlockInfo(block_header.number, block_hash));

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
    auto node = tree_->getRoot().findByHash(block_hash);
    if (!node) {
      return BlockTreeError::NON_FINALIZED_BLOCK_NOT_FOUND;
    }
    if (node->depth <= getLastFinalized().number
        && hasDirectChain(block_hash, getLastFinalized().hash)) {
      // block was already finalized, fine
      return outcome::success();
    }

    SL_DEBUG(log_,
             "Finalizing block {}",
             primitives::BlockInfo(node->depth, block_hash));

    OUTCOME_TRY(header_opt, storage_->getBlockHeader(node->block_hash));
    if (!header_opt.has_value()) {
      return BlockTreeError::HEADER_NOT_FOUND;
    }
    auto &header = header_opt.value();

    auto last_finalized_block_info =
        tree_->getMetadata().last_finalized.lock()->getBlockInfo();

    // update our local meta
    node->finalized = true;

    OUTCOME_TRY(prune(node));

    tree_->updateTreeRoot(node, justification);

    OUTCOME_TRY(reorganize());

    OUTCOME_TRY(
        storage_->setBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
                                      tree_->getMetadata().leaves.end()}));

    chain_events_engine_->notify(
        primitives::events::ChainEventType::kFinalizedHeads, header);

    OUTCOME_TRY(body, storage_->getBlockBody(node->block_hash));
    if (body.has_value()) {
      for (auto &ext : body.value()) {
        if (auto key = extrinsic_event_key_repo_->get(
                hasher_->blake2b_256(ext.data))) {
          extrinsic_events_engine_->notify(
              key.value(),
              primitives::events::ExtrinsicLifecycleEvent::Finalized(
                  key.value(), block_hash));
        }
      }
    }

    primitives::BlockInfo finalized_block(node->depth, block_hash);
    log_->info("Finalized block {}", finalized_block);
    telemetry_->notifyBlockFinalized(finalized_block);
    metric_finalized_block_height_->set(node->depth);

    KAGOME_PROFILE_START(justification_store)

    OUTCOME_TRY(
        storage_->putJustification(justification, block_hash, node->depth));
    SL_DEBUG(log_,
             "Store justification for finalized block #{} {}",
             node->depth,
             block_hash);
    // we store justification for last finalized block only as long as it is
    // last finalized (if it doesn't meet other justification storage rules,
    // e.g. its number a multiple of 512)
    OUTCOME_TRY(last_finalized_header_opt,
                storage_->getBlockHeader(last_finalized_block_info.number));
    // SAFETY: header for the last finalized block must be present
    auto &last_finalized_header = last_finalized_header_opt.value();
    OUTCOME_TRY(
        shouldStoreLastFinalized,
        justification_storage_policy_->shouldStoreFor(last_finalized_header));
    if (!shouldStoreLastFinalized) {
      OUTCOME_TRY(justification_opt,
                  storage_->getJustification(last_finalized_block_info.hash));
      if (justification_opt.has_value()) {
        SL_DEBUG(log_,
                 "Purge redundant justification for finalized block {}",
                 last_finalized_block_info);
        OUTCOME_TRY(storage_->removeJustification(
            last_finalized_block_info.hash, last_finalized_block_info.number));
      }
    }

    KAGOME_PROFILE_END(justification_store)

    return outcome::success();
  }

  outcome::result<bool> BlockTreeImpl::hasBlockHeader(
      const primitives::BlockId &block) const {
    return storage_->hasBlockHeader(block);
  }

  outcome::result<primitives::BlockHeader> BlockTreeImpl::getBlockHeader(
      const primitives::BlockId &block) const {
    OUTCOME_TRY(header, storage_->getBlockHeader(block));
    if (header.has_value()) return header.value();
    return BlockTreeError::HEADER_NOT_FOUND;
  }

  outcome::result<primitives::BlockBody> BlockTreeImpl::getBlockBody(
      const primitives::BlockId &block) const {
    OUTCOME_TRY(body, storage_->getBlockBody(block));
    if (body.has_value()) return body.value();
    return BlockTreeError::BODY_NOT_FOUND;
  }

  outcome::result<primitives::Justification>
  BlockTreeImpl::getBlockJustification(const primitives::BlockId &block) const {
    OUTCOME_TRY(justification, storage_->getJustification(block));
    if (justification.has_value()) return justification.value();
    return BlockTreeError::JUSTIFICATION_NOT_FOUND;
  }

  BlockTree::BlockHashVecRes BlockTreeImpl::getBestChainFromBlock(
      const primitives::BlockHash &block, uint64_t maximum) const {
    auto block_number_res = header_repo_->getNumberByHash(block);
    if (block_number_res.has_error()) {
      log_->error(
          "cannot retrieve block {}: {}", block, block_number_res.error());
      return BlockTreeError::HEADER_NOT_FOUND;
    }
    auto start_block_number = block_number_res.value();

    if (maximum == 1) {
      return std::vector{block};
    }

    auto best_leaf = tree_->getMetadata().best_leaf.lock();
    BOOST_ASSERT(best_leaf != nullptr);
    auto current_depth = best_leaf->depth;

    if (start_block_number >= current_depth) {
      return std::vector{block};
    }

    auto count =
        std::min<uint64_t>(current_depth - start_block_number + 1, maximum);

    primitives::BlockNumber finish_block_number =
        start_block_number + count - 1;

    auto finish_block_hash_res =
        header_repo_->getHashByNumber(finish_block_number);
    if (finish_block_hash_res.has_error()) {
      log_->error("cannot retrieve block with number {}: {}",
                  finish_block_number,
                  finish_block_hash_res.error());
      return BlockTreeError::HEADER_NOT_FOUND;
    }
    const auto &finish_block_hash = finish_block_hash_res.value();

    OUTCOME_TRY(chain, getDescendingChainToBlock(finish_block_hash, count));
    if (chain.back() != block) {
      return std::vector{block};
    }
    std::reverse(chain.begin(), chain.end());
    return std::move(chain);
  }

  BlockTree::BlockHashVecRes BlockTreeImpl::getDescendingChainToBlock(
      const primitives::BlockHash &to_block, uint64_t maximum) const {
    std::vector<primitives::BlockHash> chain;

    auto hash = to_block;

    // Try to retrieve from cached tree
    if (auto node = tree_->getRoot().findByHash(hash)) {
      while (maximum > chain.size()) {
        auto parent = node->parent.lock();
        if (not parent) {
          hash = node->block_hash;
          break;
        }
        chain.emplace_back(node->block_hash);
        node = parent;
      }
    }

    while (maximum > chain.size()) {
      auto header_res = header_repo_->getBlockHeader(hash);
      if (header_res.has_error()) {
        if (chain.empty()) {
          log_->error("cannot retrieve block with hash {}: {}",
                      hash,
                      header_res.error());
          return BlockTreeError::HEADER_NOT_FOUND;
        }
        break;
      }
      const auto &header = header_res.value();

      chain.emplace_back(hash);

      if (header.number == 0) {
        break;
      }

      hash = header.parent_hash;
    }

    return chain;
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlocks(
      const primitives::BlockHash &ancestor,
      const primitives::BlockHash &descendant) const {
    OUTCOME_TRY(from, header_repo_->getNumberByHash(ancestor));
    OUTCOME_TRY(to, header_repo_->getNumberByHash(descendant));
    if (to < from) {
      return BlockTreeError::TARGET_IS_PAST_MAX;
    }
    auto count = to - from + 1;
    OUTCOME_TRY(chain, getDescendingChainToBlock(descendant, count));
    BOOST_ASSERT(chain.size() == count);
    if (chain.back() != ancestor) {
      return BlockTreeError::BLOCK_ON_DEAD_END;
    }
    std::reverse(chain.begin(), chain.end());
    return std::move(chain);
  }

  bool BlockTreeImpl::hasDirectChain(
      const primitives::BlockHash &ancestor,
      const primitives::BlockHash &descendant) const {
    auto ancestor_node_ptr = tree_->getRoot().findByHash(ancestor);
    auto descendant_node_ptr = tree_->getRoot().findByHash(descendant);

    /*
     * check that ancestor is above descendant
     * optimization that prevents reading blockDB up the genesis
     * TODO (xDimon) it could be not right place for this check
     *  or changing logic may make it obsolete
     *  block numbers may be obtained somewhere else
     */
    primitives::BlockNumber ancestor_depth = 0u;
    primitives::BlockNumber descendant_depth = 0u;
    if (ancestor_node_ptr) {
      ancestor_depth = ancestor_node_ptr->depth;
    } else {
      auto number_res = header_repo_->getNumberByHash(ancestor);
      if (!number_res) {
        return false;
      }
      ancestor_depth = number_res.value();
    }
    if (descendant_node_ptr) {
      descendant_depth = descendant_node_ptr->depth;
    } else {
      auto number_res = header_repo_->getNumberByHash(descendant);
      if (!number_res) {
        return false;
      }
      descendant_depth = number_res.value();
    }
    if (descendant_depth < ancestor_depth) {
      SL_DEBUG(log_,
               "Ancestor block is lower. {} in comparison with {}",
               primitives::BlockInfo(ancestor_depth, ancestor),
               primitives::BlockInfo(descendant_depth, descendant));
      return false;
    }

    // if both nodes are in our light tree, we can use this representation
    // only
    if (ancestor_node_ptr && descendant_node_ptr) {
      auto current_node = descendant_node_ptr;
      while (current_node != ancestor_node_ptr) {
        if (current_node->depth <= ancestor_node_ptr->depth) {
          return false;
        }
        if (auto parent = current_node->parent; !parent.expired()) {
          current_node = parent.lock();
        } else {
          return false;
        }
      }
      return true;
    }

    // else, we need to use a database

    // Try to use optimal way, if ancestor and descendant in the finalized chain
    if (descendant_depth <= getLastFinalized().number) {
      auto res = header_repo_->getHashByNumber(descendant_depth);
      BOOST_ASSERT_MSG(res.has_value(),
                       "Any finalized block must be accessible by number");
      // Check if descendant in finalised chain
      if (res.value() == descendant) {
        res = header_repo_->getHashByNumber(ancestor_depth);
        BOOST_ASSERT_MSG(res.has_value(),
                         "Any finalized block must be accessible by number");
        if (res.value() == ancestor) {
          // Ancestor and descendant in the finalized chain,
          // therefore they have direct chain between each other
          return true;
        } else {
          // Ancestor in the finalized chain, but descendant is not,
          // therefore they can not have direct chain between each other
          return false;
        }
      }
    }

    auto current_hash = descendant;
    KAGOME_PROFILE_START(search_finalized_chain)
    while (current_hash != ancestor) {
      auto current_header_res = header_repo_->getBlockHeader(current_hash);
      if (!current_header_res) {
        return false;
      }
      if (current_header_res.value().number <= ancestor_depth) {
        return false;
      }
      current_hash = current_header_res.value().parent_hash;
    }
    KAGOME_PROFILE_END(search_finalized_chain)
    return true;
  }

  primitives::BlockInfo BlockTreeImpl::bestLeaf() const {
    auto &&leaf = tree_->getMetadata().best_leaf.lock();
    BOOST_ASSERT(leaf != nullptr);
    return {leaf->depth, leaf->block_hash};
  }

  outcome::result<primitives::BlockInfo> BlockTreeImpl::getBestContaining(
      const primitives::BlockHash &target_hash,
      const std::optional<primitives::BlockNumber> &max_number) const {
    OUTCOME_TRY(target_header, header_repo_->getBlockHeader(target_hash));
    if (max_number.has_value() && target_header.number > max_number.value()) {
      return BlockTreeError::TARGET_IS_PAST_MAX;
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
                  header_repo_->getNumberByHash(getLastFinalized().hash));
      if (last_finalized >= target_header.number) {
        return BlockTreeError::BLOCK_ON_DEAD_END;
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
      primitives::BlockNumber current_block_number{};
      do {
        OUTCOME_TRY(current_header, header_repo_->getBlockHeader(current_hash));
        if (current_hash == target_hash) {
          return primitives::BlockInfo{best_header.number, best_hash};
        }
        current_block_number = current_header.number;
        current_hash = current_header.parent_hash;
      } while (current_block_number >= target_header.number);
    }

    log_->warn(
        "Block {} exists in chain but not found when following all leaves "
        "backwards. Max block number = {}",
        target_hash,
        max_number);
    return BlockTreeError::EXISTING_BLOCK_NOT_FOUND;
  }

  std::vector<primitives::BlockHash> BlockTreeImpl::getLeaves() const {
    std::vector<primitives::BlockHash> result;
    result.reserve(tree_->getMetadata().leaves.size());
    std::transform(tree_->getMetadata().leaves.begin(),
                   tree_->getMetadata().leaves.end(),
                   std::back_inserter(result),
                   [](const auto &hash) { return hash; });
    return result;
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChildren(
      const primitives::BlockHash &block) const {
    if (auto node = tree_->getRoot().findByHash(block); node != nullptr) {
      std::vector<primitives::BlockHash> result;
      result.reserve(node->children.size());
      for (const auto &child : node->children) {
        result.push_back(child->block_hash);
      }
      return result;
    }
    OUTCOME_TRY(header, storage_->getBlockHeader(block));
    if (!header.has_value()) return BlockTreeError::HEADER_NOT_FOUND;
    // if node is not in tree_ it must be finalized and thus have only one child
    OUTCOME_TRY(child_hash,
                header_repo_->getHashByNumber(header.value().number + 1));
    return outcome::success(std::vector<primitives::BlockHash>{child_hash});
  }

  primitives::BlockInfo BlockTreeImpl::getLastFinalized() const {
    const auto &last = tree_->getMetadata().last_finalized.lock();
    BOOST_ASSERT(last != nullptr);
    return primitives::BlockInfo{last->depth, last->block_hash};
  }

  std::vector<primitives::BlockHash> BlockTreeImpl::getLeavesSorted() const {
    std::vector<primitives::BlockInfo> leaf_depths;
    auto leaves = getLeaves();
    leaf_depths.reserve(leaves.size());
    for (auto &leaf : leaves) {
      auto leaf_node = tree_->getRoot().findByHash(leaf);
      leaf_depths.emplace_back(
          primitives::BlockInfo{leaf_node->depth, leaf_node->block_hash});
    }
    std::sort(
        leaf_depths.begin(),
        leaf_depths.end(),
        [](auto const &p1, auto const &p2) { return p1.number > p2.number; });
    std::vector<primitives::BlockHash> leaf_hashes;
    leaf_hashes.reserve(leaf_depths.size());
    std::transform(leaf_depths.begin(),
                   leaf_depths.end(),
                   std::back_inserter(leaf_hashes),
                   [](auto &p) { return p.hash; });
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
    std::deque<std::shared_ptr<TreeNode>> to_remove;

    auto following_node = lastFinalizedNode;

    for (auto current_node = following_node->parent.lock();
         current_node && !current_node->finalized;
         current_node = current_node->parent.lock()) {
      // DFS-on-deque
      to_remove.emplace_back();  // Waterbreak
      std::copy_if(current_node->children.begin(),
                   current_node->children.end(),
                   std::back_inserter(to_remove),
                   [&](const auto &child) { return child != following_node; });
      auto last = to_remove.back();
      while (last != nullptr) {
        to_remove.pop_back();
        std::copy(last->children.begin(),
                  last->children.end(),
                  std::back_inserter(to_remove));
        to_remove.emplace_front(std::move(last));
        last = to_remove.back();
      }
      to_remove.pop_back();  // Remove waterbreak

      // remove (in memory) all child, except main chain block
      current_node->children = {following_node};
      following_node = current_node;
    }

    std::vector<primitives::Extrinsic> extrinsics;

    // remove from storage
    for (const auto &node : to_remove) {
      OUTCOME_TRY(block_body_res, storage_->getBlockBody(node->block_hash));
      if (block_body_res.has_value()) {
        extrinsics.reserve(extrinsics.size() + block_body_res.value().size());
        for (auto &ext : block_body_res.value()) {
          if (auto key = extrinsic_event_key_repo_->get(
                  hasher_->blake2b_256(ext.data))) {
            extrinsic_events_engine_->notify(
                key.value(),
                primitives::events::ExtrinsicLifecycleEvent::Retracted(
                    key.value(), node->block_hash));
          }
          extrinsics.emplace_back(std::move(ext));
        }
      }

      tree_->removeFromMeta(node);
      OUTCOME_TRY(storage_->removeBlock({node->depth, node->block_hash}));
    }

    // trying to return extrinsics back to transaction pool
    for (auto &&extrinsic : extrinsics) {
      auto result = extrinsic_observer_->onTxMessage(extrinsic);
      if (result) {
        SL_DEBUG(log_, "Tx {} was reapplied", result.value().toHex());
      } else {
        SL_DEBUG(log_, "Tx was skipped: {}", result.error());
      }
    }

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::reorganize() {
    auto block = BlockTreeImpl::bestLeaf();
    if (block.number == 0) {
      return outcome::success();
    }
    auto hash_res = header_repo_->getHashByNumber(block.number);
    if (hash_res.has_error()) {
      if (hash_res
          != outcome::failure(blockchain::BlockTreeError::HEADER_NOT_FOUND)) {
        return hash_res.as_failure();
      }
    } else if (block.hash == hash_res.value()) {
      return outcome::success();
    }

    size_t count = 0;
    for (;;) {
      OUTCOME_TRY(storage_->putNumberToIndexKey(block));
      if (block.number == 0) break;
      OUTCOME_TRY(header, getBlockHeader(block.hash));
      auto parent_hash_res = header_repo_->getHashByNumber(block.number - 1);
      if (parent_hash_res.has_error()) {
        if (parent_hash_res
            != outcome::failure(blockchain::BlockTreeError::HEADER_NOT_FOUND)) {
          return parent_hash_res.as_failure();
        }
      } else if (header.parent_hash == parent_hash_res.value()) {
        break;
      }
      ++count;
      block = {block.number - 1, header.parent_hash};
    }

    if (count > 1) {
      SL_DEBUG(log_, "Best chain reorganized for {} blocks deep", count);
    }

    return outcome::success();
  }

}  // namespace kagome::blockchain
