/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_tree_impl.hpp"

#include <algorithm>
#include <set>
#include <stack>

#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/cached_tree.hpp"
#include "blockchain/impl/justification_storage_policy.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/is_primary.hpp"
#include "crypto/blake2/blake2b.h"
#include "log/formatters/optional.hpp"
#include "log/profiling_logger.hpp"
#include "storage/database_error.hpp"
#include "storage/trie_pruner/trie_pruner.hpp"

namespace {
  constexpr auto blockHeightMetricName = "kagome_block_height";
  constexpr auto knownChainLeavesMetricName = "kagome_number_leaves";
}  // namespace

namespace kagome::blockchain {
  using Buffer = common::Buffer;
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
            if (res == outcome::failure(BlockTreeError::HEADER_NOT_FOUND)) {
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

          auto hash_opt_res = storage->getBlockHash(number);
          if (hash_opt_res.has_failure()) {
            SL_CRITICAL(
                log, "Search best block has failed: {}", hash_opt_res.error());
            return BlockTreeError::HEADER_NOT_FOUND;
          }
          const auto &hash_opt = hash_opt_res.value();

          if (hash_opt.has_value()) {
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

  BlockTreeImpl::SafeBlockTreeData::SafeBlockTreeData(BlockTreeData data)
      : block_tree_data_{std::move(data)} {}

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
      std::shared_ptr<const class JustificationStoragePolicy>
          justification_storage_policy,
      std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner,
      std::shared_ptr<::boost::asio::io_context> io_context) {
    BOOST_ASSERT(storage != nullptr);
    BOOST_ASSERT(header_repo != nullptr);

    log::Logger log = log::createLogger("BlockTree", "block_tree");

    OUTCOME_TRY(last_finalized_block_info, storage->getLastFinalized());

    auto finalized_block_header_res =
        storage->getBlockHeader(last_finalized_block_info.hash);
    BOOST_ASSERT_MSG(finalized_block_header_res.has_value()
                         and finalized_block_header_res.value().has_value(),
                     "Initialized block tree must be have finalized block");
    auto &finalized_block_header = finalized_block_header_res.value().value();
    // call chain_events_engine->notify to init babe_config_repo preventive
    chain_events_engine->notify(
        primitives::events::ChainEventType::kFinalizedHeads,
        finalized_block_header);

    OUTCOME_TRY(storage->getJustification(last_finalized_block_info.hash));

    OUTCOME_TRY(block_tree_leaves, loadLeaves(storage, header_repo, log));
    BOOST_ASSERT_MSG(not block_tree_leaves.empty(),
                     "Must be known or calculated at least one leaf");

    auto highest_leaf = *block_tree_leaves.rbegin();
    SL_INFO(log,
            "Highest block: {}, Last finalized: {}",
            highest_leaf,
            last_finalized_block_info);

    // Load non-finalized block from block storage
    std::map<primitives::BlockInfo, primitives::BlockHeader> collected;

    {
      std::unordered_set<primitives::BlockHash> observed;
      std::unordered_set<primitives::BlockInfo> dead;
      // Iterate leaves
      for (auto &leaf : block_tree_leaves) {
        std::unordered_set<primitives::BlockInfo> subchain;
        // Iterate subchain from leaf to finalized or early observer
        for (auto block = leaf;;) {
          // Met last finalized
          if (block.hash == last_finalized_block_info.hash) {
            break;
          }

          // Met early observed block
          if (observed.count(block.hash) != 0) {
            break;
          }

          // Met known dead block
          if (dead.count(block) != 0) {
            dead.insert(subchain.begin(), subchain.end());
            break;
          }

          // Check if non-pruned fork has detected
          if (block.number == last_finalized_block_info.number) {
            dead.insert(subchain.begin(), subchain.end());

            auto main = last_finalized_block_info;
            auto fork = block;

            // Collect as dead all blocks that differ from the finalized chain
            for (;;) {
              dead.emplace(fork);

              auto f_res = storage->getBlockHeader(fork.hash);
              if (f_res.has_error() or not f_res.value().has_value()) {
                break;
              }
              const auto &fork_header = f_res.value().value();

              auto m_res = storage->getBlockHeader(main.hash);
              if (m_res.has_error() or not m_res.value().has_value()) {
                break;
              }
              const auto &main_header = m_res.value().value();

              BOOST_ASSERT(fork_header.number == main_header.number);
              if (fork_header.parent_hash == main_header.parent_hash) {
                break;
              }

              fork = *fork_header.parentInfo();
              main = *main_header.parentInfo();
            }

            break;
          };

          subchain.emplace(block);

          auto header_res = storage->getBlockHeader(block.hash);
          if (header_res.has_error()) {
            SL_WARN(log,
                    "Can't get header of existing non-finalized block {}: {}",
                    block,
                    header_res.error());
            return header_res.as_failure();
          }

          auto &header_opt = header_res.value();
          if (not header_opt.has_value()) {
            SL_WARN(log,
                    "Can't get header of existing block {}: "
                    "not found in block storage",
                    block);
            dead.insert(subchain.begin(), subchain.end());
            break;
          }

          observed.emplace(block.hash);

          auto &header = header_opt.value();
          if (header.number < last_finalized_block_info.number) {
            SL_WARN(
                log,
                "Detected a leaf {} lower than the last finalized block #{}",
                block,
                last_finalized_block_info.number);
            break;
          }

          auto [it, ok] = collected.emplace(block, std::move(header));

          block = *it->second.parentInfo();
        }
      }

      if (not dead.empty()) {
        SL_WARN(log,
                "Found {} orphan blocks; "
                "these block will be removed for consistency",
                dead.size());
        for (auto &block : dead) {
          collected.erase(block);
          std::ignore = storage->removeBlock(block.hash);
        }
      }
    }

    // Prepare and create block tree basing last finalized block
    SL_DEBUG(log, "Last finalized block {}", last_finalized_block_info);

    std::shared_ptr<BlockTreeImpl> block_tree(new BlockTreeImpl(
        std::move(header_repo),
        std::move(storage),
        std::make_unique<CachedTree>(last_finalized_block_info),
        std::move(extrinsic_observer),
        std::move(hasher),
        std::move(chain_events_engine),
        std::move(extrinsic_events_engine),
        std::move(extrinsic_event_key_repo),
        std::move(justification_storage_policy),
        state_pruner,
        std::move(io_context)));

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

    OUTCOME_TRY(state_pruner->recoverState(*block_tree));

    return block_tree;
  }

  outcome::result<void> BlockTreeImpl::recover(
      primitives::BlockId target_block_id,
      std::shared_ptr<BlockStorage> storage,
      std::shared_ptr<BlockHeaderRepository> header_repo,
      std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<BlockTree> block_tree) {
    BOOST_ASSERT(storage != nullptr);
    BOOST_ASSERT(header_repo != nullptr);
    BOOST_ASSERT(trie_storage != nullptr);

    log::Logger log = log::createLogger("BlockTree", "block_tree");

    OUTCOME_TRY(block_tree_leaves, loadLeaves(storage, header_repo, log));

    BOOST_ASSERT_MSG(not block_tree_leaves.empty(),
                     "Must be known or calculated at least one leaf");

    auto target_block_hash_opt_res = storage->getBlockHash(target_block_id);
    if (target_block_hash_opt_res.has_failure()) {
      SL_CRITICAL(log,
                  "Can't get header of target block: {}",
                  target_block_hash_opt_res.error());
      return BlockTreeError::HEADER_NOT_FOUND;
    } else if (not target_block_hash_opt_res.value().has_value()) {
      SL_CRITICAL(log, "Can't get header of target block: header not found");
      return BlockTreeError::HEADER_NOT_FOUND;
    }
    const auto &target_block_hash = target_block_hash_opt_res.value().value();

    // Check if target block exists
    auto target_block_header_opt_res =
        storage->getBlockHeader(target_block_hash);
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
    if (auto res = trie_storage->getEphemeralBatchAt(state_root);
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
      block_tree_leaves.emplace(*header.parentInfo());
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
      std::shared_ptr<const JustificationStoragePolicy>
          justification_storage_policy,
      std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner,
      std::shared_ptr<::boost::asio::io_context> io_context)
      : block_tree_data_{BlockTreeData{
          .header_repo_ = std::move(header_repo),
          .storage_ = std::move(storage),
          .state_pruner_ = std::move(state_pruner),
          .tree_ = std::move(cached_tree),
          .extrinsic_observer_ = std::move(extrinsic_observer),
          .hasher_ = std::move(hasher),
          .extrinsic_event_key_repo_ = std::move(extrinsic_event_key_repo),
          .justification_storage_policy_ =
              std::move(justification_storage_policy),
      }},
        main_thread_{std::move(io_context)} {
    block_tree_data_.sharedAccess([&](const BlockTreeData &p) {
      BOOST_ASSERT(p.header_repo_ != nullptr);
      BOOST_ASSERT(p.storage_ != nullptr);
      BOOST_ASSERT(p.tree_ != nullptr);
      BOOST_ASSERT(p.extrinsic_observer_ != nullptr);
      BOOST_ASSERT(p.hasher_ != nullptr);
      BOOST_ASSERT(p.extrinsic_event_key_repo_ != nullptr);
      BOOST_ASSERT(p.justification_storage_policy_ != nullptr);
      BOOST_ASSERT(p.state_pruner_ != nullptr);

      // Register metrics
      BOOST_ASSERT(telemetry_ != nullptr);
      metrics_registry_->registerGaugeFamily(blockHeightMetricName,
                                             "Block height info of the chain");

      metric_best_block_height_ = metrics_registry_->registerGaugeMetric(
          blockHeightMetricName, {{"status", "best"}});
      metric_best_block_height_->set(bestBlockNoLock(p).number);

      metric_finalized_block_height_ = metrics_registry_->registerGaugeMetric(
          blockHeightMetricName, {{"status", "finalized"}});
      metric_finalized_block_height_->set(getLastFinalizedNoLock(p).number);

      metrics_registry_->registerGaugeFamily(
          knownChainLeavesMetricName,
          "Number of known chain leaves (aka forks)");

      metric_known_chain_leaves_ =
          metrics_registry_->registerGaugeMetric(knownChainLeavesMetricName);
      metric_known_chain_leaves_->set(p.tree_->leafCount());

      telemetry_->setGenesisBlockHash(getGenesisBlockHash());
    });

    chain_events_engine_ = std::move(chain_events_engine);
    BOOST_ASSERT(chain_events_engine_ != nullptr);

    extrinsic_events_engine_ = std::move(extrinsic_events_engine);
    BOOST_ASSERT(extrinsic_events_engine_ != nullptr);

    main_thread_.start();
  }

  const primitives::BlockHash &BlockTreeImpl::getGenesisBlockHash() const {
    return block_tree_data_
        .sharedAccess(
            [&](const BlockTreeData &p)
                -> std::reference_wrapper<const primitives::BlockHash> {
              if (p.genesis_block_hash_.has_value()) {
                return p.genesis_block_hash_.value();
              }

              auto res = p.header_repo_->getHashByNumber(0);
              BOOST_ASSERT_MSG(
                  res.has_value(),
                  "Block tree must contain at least genesis block");

              const_cast<std::decay_t<decltype(p.genesis_block_hash_)> &>(
                  p.genesis_block_hash_)
                  .emplace(res.value());
              return p.genesis_block_hash_.value();
            })
        .get();
  }

  outcome::result<void> BlockTreeImpl::addBlockHeader(
      const primitives::BlockHeader &header) {
    return block_tree_data_.exclusiveAccess(
        [&](BlockTreeData &p) -> outcome::result<void> {
          auto parent = p.tree_->getRoot().findByHash(header.parent_hash);
          if (!parent) {
            return BlockTreeError::NO_PARENT;
          }
          OUTCOME_TRY(block_hash, p.storage_->putBlockHeader(header));

          // update local meta with the new block
          auto new_node = std::make_shared<TreeNode>(
              header.blockInfo(), parent, isPrimary(header));

          p.tree_->updateMeta(new_node);

          OUTCOME_TRY(reorganizeNoLock(p));

          OUTCOME_TRY(p.storage_->setBlockTreeLeaves(p.tree_->leafHashes()));

          metric_known_chain_leaves_->set(p.tree_->leafCount());
          metric_best_block_height_->set(bestBlockNoLock(p).number);

          notifyChainEventsEngine(primitives::events::ChainEventType::kNewHeads,
                                  header);
          SL_VERBOSE(log_,
                     "Block {} has been added into block tree",
                     primitives::BlockInfo(header.number, block_hash));

          return outcome::success();
        });
  }

  outcome::result<void> BlockTreeImpl::addBlock(
      const primitives::Block &block) {
    return block_tree_data_.exclusiveAccess(
        [&](BlockTreeData &p) -> outcome::result<void> {
          // Check if we know parent of this block; if not, we cannot insert it
          auto parent = p.tree_->getRoot().findByHash(block.header.parent_hash);
          if (!parent) {
            return BlockTreeError::NO_PARENT;
          }

          // Save block
          OUTCOME_TRY(block_hash, p.storage_->putBlock(block));

          // Update local meta with the block
          auto new_node = std::make_shared<TreeNode>(
              block.header.blockInfo(), parent, isPrimary(block.header));

          p.tree_->updateMeta(new_node);

          OUTCOME_TRY(reorganizeNoLock(p));

          OUTCOME_TRY(p.storage_->setBlockTreeLeaves(p.tree_->leafHashes()));

          notifyChainEventsEngine(primitives::events::ChainEventType::kNewHeads,
                                  block.header);
          SL_DEBUG(log_, "Adding block {}", block_hash);
          for (const auto &ext : block.body) {
            auto extrinsic_hash = p.hasher_->blake2b_256(ext.data);
            SL_DEBUG(log_, "Adding extrinsic with hash {}", extrinsic_hash);
            if (auto key = p.extrinsic_event_key_repo_->get(extrinsic_hash)) {
              main_thread_.execute(
                  [wself{weak_from_this()}, key{key.value()}, block_hash]() {
                    if (auto self = wself.lock()) {
                      self->extrinsic_events_engine_->notify(
                          key,
                          primitives::events::ExtrinsicLifecycleEvent::InBlock(
                              key, block_hash));
                    }
                  });
            }
          }

          metric_known_chain_leaves_->set(p.tree_->leafCount());
          metric_best_block_height_->set(bestBlockNoLock(p).number);

          SL_VERBOSE(log_,
                     "Block {} has been added into block tree",
                     primitives::BlockInfo(block.header.number, block_hash));
          return outcome::success();
        });
  }

  void BlockTreeImpl::notifyChainEventsEngine(
      primitives::events::ChainEventType event,
      const primitives::BlockHeader &header) {
    BOOST_ASSERT(header.hash_opt.has_value());
    main_thread_.execute([wself{weak_from_this()}, event, header]() mutable {
      if (auto self = wself.lock()) {
        self->chain_events_engine_->notify(std::move(event), std::move(header));
      }
    });
  }

  outcome::result<void> BlockTreeImpl::removeLeaf(
      const primitives::BlockHash &block_hash) {
    return block_tree_data_.exclusiveAccess(
        [&](BlockTreeData &p) -> outcome::result<void> {
          // Check if block is leaf
          if (block_hash == getLastFinalizedNoLock(p).hash
              or p.tree_->isLeaf(block_hash)) {
            return BlockTreeError::BLOCK_IS_NOT_LEAF;
          }

          auto node = p.tree_->getRoot().findByHash(block_hash);
          BOOST_ASSERT_MSG(node != nullptr,
                           "As checked before, block exists as one of leaves");

          p.tree_->removeFromMeta(node);

          OUTCOME_TRY(reorganizeNoLock(p));

          // Remove from storage
          OUTCOME_TRY(p.storage_->removeBlock(node->block_hash));

          OUTCOME_TRY(p.storage_->setBlockTreeLeaves(p.tree_->leafHashes()));

          return outcome::success();
        });
  }

  outcome::result<void> BlockTreeImpl::markAsParachainDataBlock(
      const primitives::BlockHash &block_hash) {
    return block_tree_data_.exclusiveAccess(
        [&](BlockTreeData &p) -> outcome::result<void> {
          SL_TRACE(log_, "Trying to adjust weight for block {}", block_hash);

          auto node = p.tree_->getRoot().findByHash(block_hash);
          if (node == nullptr) {
            SL_WARN(log_, "Block {} doesn't exists in block tree", block_hash);
            return BlockTreeError::BLOCK_NOT_EXISTS;
          }

          node->contains_approved_para_block = true;
          return outcome::success();
        });
  }

  outcome::result<void> BlockTreeImpl::markAsRevertedBlocks(
      const std::vector<primitives::BlockHash> &block_hashes) {
    return block_tree_data_.exclusiveAccess(
        [&](BlockTreeData &p) -> outcome::result<void> {
          bool need_to_refresh_best = false;
          auto best = bestBlockNoLock(p);
          for (const auto &block_hash : block_hashes) {
            auto tree_node = p.tree_->getRoot().findByHash(block_hash);
            if (tree_node == nullptr) {
              SL_WARN(
                  log_, "Block {} doesn't exists in block tree", block_hash);
              continue;
            }

            if (not tree_node->reverted) {
              std::queue<std::shared_ptr<TreeNode>> to_revert;
              to_revert.push(std::move(tree_node));
              while (not to_revert.empty()) {
                auto &reverting_tree_node = to_revert.front();

                reverting_tree_node->reverted = true;

                if (reverting_tree_node->getBlockInfo() == best) {
                  need_to_refresh_best = true;
                }

                for (auto &child : reverting_tree_node->children) {
                  if (not child->reverted) {
                    to_revert.push(child);
                  }
                }

                to_revert.pop();
              }
            }
          }
          if (need_to_refresh_best) {
            p.tree_->forceRefreshBest();
          }
          return outcome::success();
        });
  }

  outcome::result<void> BlockTreeImpl::addExistingBlockNoLock(
      BlockTreeData &p,
      const primitives::BlockHash &block_hash,
      const primitives::BlockHeader &block_header) {
    SL_TRACE(log_,
             "Trying to add block {} into block tree",
             primitives::BlockInfo(block_header.number, block_hash));

    auto node = p.tree_->getRoot().findByHash(block_hash);
    // Check if tree doesn't have this block; if not, we skip that
    if (node != nullptr) {
      SL_TRACE(log_,
               "Block {} exists in block tree",
               primitives::BlockInfo(block_header.number, block_hash));
      return BlockTreeError::BLOCK_EXISTS;
    }

    auto parent = p.tree_->getRoot().findByHash(block_header.parent_hash);

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

      auto finalized = getLastFinalizedNoLock(p).number;

      for (auto hash = block_header.parent_hash;;) {
        OUTCOME_TRY(header_opt, p.storage_->getBlockHeader(hash));
        if (not header_opt.has_value()) {
          return BlockTreeError::NO_PARENT;
        }

        auto &header = header_opt.value();
        SL_TRACE(log_,
                 "Block {} has found in storage and enqueued to add",
                 primitives::BlockInfo(header.number, hash));

        if (header.number <= finalized) {
          return BlockTreeError::BLOCK_ON_DEAD_END;
        }

        to_add.emplace(hash, std::move(header));

        if (p.tree_->getRoot().findByHash(header.parent_hash) != nullptr) {
          SL_TRACE(log_,
                   "Block {} parent of {} has found in block tree",
                   primitives::BlockInfo(*header.parentInfo()),
                   primitives::BlockInfo(header.number, hash));

          break;
        }

        SL_TRACE(log_,
                 "Block {} has not found in block tree. "
                 "Trying to restore from storage",
                 *header.parentInfo());

        hash = header.parent_hash;
      }

      while (not to_add.empty()) {
        const auto &[hash, header] = to_add.top();
        OUTCOME_TRY(addExistingBlockNoLock(p, hash, header));
        to_add.pop();
      }

      parent = p.tree_->getRoot().findByHash(block_header.parent_hash);
      BOOST_ASSERT_MSG(parent != nullptr,
                       "Parent must be restored at this moment");

      SL_TRACE(log_,
               "Trying to add block {} into block tree",
               primitives::BlockInfo(block_header.number, block_hash));
    }

    // Update local meta with the block
    auto new_node = std::make_shared<TreeNode>(
        block_header.blockInfo(), parent, isPrimary(block_header));

    p.tree_->updateMeta(new_node);

    OUTCOME_TRY(reorganizeNoLock(p));

    OUTCOME_TRY(p.storage_->setBlockTreeLeaves(p.tree_->leafHashes()));

    metric_known_chain_leaves_->set(p.tree_->leafCount());
    metric_best_block_height_->set(bestBlockNoLock(p).number);

    SL_VERBOSE(log_,
               "Block {} has been restored in block tree from storage",
               primitives::BlockInfo(block_header.number, block_hash));

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::addExistingBlock(
      const primitives::BlockHash &block_hash,
      const primitives::BlockHeader &block_header) {
    return block_tree_data_.exclusiveAccess(
        [&](BlockTreeData &p) -> outcome::result<void> {
          return addExistingBlockNoLock(p, block_hash, block_header);
        });
  }

  outcome::result<void> BlockTreeImpl::addBlockBody(
      const primitives::BlockHash &block_hash,
      const primitives::BlockBody &body) {
    return block_tree_data_.exclusiveAccess(
        [&](BlockTreeData &p) -> outcome::result<void> {
          return p.storage_->putBlockBody(block_hash, body);
        });
  }

  outcome::result<void> BlockTreeImpl::finalize(
      const primitives::BlockHash &block_hash,
      const primitives::Justification &justification) {
    return block_tree_data_.exclusiveAccess([&](BlockTreeData &p)
                                                -> outcome::result<void> {
      const auto node = p.tree_->getRoot().findByHash(block_hash);
      if (!node) {
        return BlockTreeError::NON_FINALIZED_BLOCK_NOT_FOUND;
      }
      const auto block = node->getBlockInfo();

      auto last_finalized_block_info = getLastFinalizedNoLock(p);

      auto justification_stored = false;

      if (node->depth > last_finalized_block_info.number) {
        SL_DEBUG(log_, "Finalizing block {}", block);

        OUTCOME_TRY(header_opt, p.storage_->getBlockHeader(node->block_hash));
        if (not header_opt.has_value()) {
          return BlockTreeError::HEADER_NOT_FOUND;
        }
        auto &header = header_opt.value();

        OUTCOME_TRY(p.storage_->putJustification(justification, block_hash));
        justification_stored = true;

        OUTCOME_TRY(pruneNoLock(p, node));
        OUTCOME_TRY(pruneTrie(p, node->getBlockInfo().number));

        p.tree_->updateTreeRoot(node);

        OUTCOME_TRY(reorganizeNoLock(p));

        OUTCOME_TRY(p.storage_->setBlockTreeLeaves(p.tree_->leafHashes()));

        notifyChainEventsEngine(
            primitives::events::ChainEventType::kFinalizedHeads, header);

        OUTCOME_TRY(body, p.storage_->getBlockBody(node->block_hash));
        if (body.has_value()) {
          for (auto &ext : body.value()) {
            auto extrinsic_hash = p.hasher_->blake2b_256(ext.data);
            if (auto key = p.extrinsic_event_key_repo_->get(extrinsic_hash)) {
              main_thread_.execute([wself{weak_from_this()},
                                    key{key.value()},
                                    block_hash]() {
                if (auto self = wself.lock()) {
                  self->extrinsic_events_engine_->notify(
                      key,
                      primitives::events::ExtrinsicLifecycleEvent::Finalized(
                          key, block_hash));
                }
              });
            }
          }
        }

        log_->info("Finalized block {}", block);
        telemetry_->notifyBlockFinalized(block);
        telemetry_->pushBlockStats();
        metric_finalized_block_height_->set(node->depth);

      } else if (node->block_hash == last_finalized_block_info.hash) {
        // block is current last finalized, fine
        return outcome::success();
      } else if (hasDirectChainNoLock(
                     p, block_hash, last_finalized_block_info.hash)) {
        OUTCOME_TRY(justification_opt,
                    p.storage_->getJustification(block_hash));
        if (justification_opt.has_value()) {
          // block already has justification (in DB), fine
          return outcome::success();
        }
      }

      KAGOME_PROFILE_START(justification_store)

      if (not justification_stored) {
        OUTCOME_TRY(p.storage_->putJustification(justification, block_hash));
      }
      SL_DEBUG(log_, "Store justification for finalized block {}", block);

      if (last_finalized_block_info.number < node->depth) {
        // we store justification for last finalized block only as long as it is
        // last finalized (if it doesn't meet other justification storage rules,
        // e.g. its number a multiple of 512)
        OUTCOME_TRY(last_finalized_header_opt,
                    p.storage_->getBlockHeader(last_finalized_block_info.hash));
        // SAFETY: header for the last finalized block must be present
        auto &last_finalized_header = last_finalized_header_opt.value();
        OUTCOME_TRY(
            shouldStoreLastFinalized,
            p.justification_storage_policy_->shouldStoreFor(
                last_finalized_header, getLastFinalizedNoLock(p).number));
        if (!shouldStoreLastFinalized) {
          OUTCOME_TRY(
              justification_opt,
              p.storage_->getJustification(last_finalized_block_info.hash));
          if (justification_opt.has_value()) {
            SL_DEBUG(log_,
                     "Purge redundant justification for finalized block {}",
                     last_finalized_block_info);
            OUTCOME_TRY(p.storage_->removeJustification(
                last_finalized_block_info.hash));
          }
        }
      }

      KAGOME_PROFILE_END(justification_store)

      return outcome::success();
    });
  }

  outcome::result<std::optional<primitives::BlockHash>>
  BlockTreeImpl::getBlockHash(primitives::BlockNumber block_number) const {
    return block_tree_data_.sharedAccess(
        [&](const BlockTreeData &p)
            -> outcome::result<std::optional<primitives::BlockHash>> {
          OUTCOME_TRY(hash_opt, p.storage_->getBlockHash(block_number));
          return hash_opt;
        });
  }

  outcome::result<bool> BlockTreeImpl::hasBlockHeader(
      const primitives::BlockHash &block_hash) const {
    return block_tree_data_.sharedAccess([&](const BlockTreeData &p) {
      return p.storage_->hasBlockHeader(block_hash);
    });
  }

  outcome::result<primitives::BlockHeader> BlockTreeImpl::getBlockHeaderNoLock(
      const BlockTreeData &p, const primitives::BlockHash &block_hash) const {
    OUTCOME_TRY(header, p.storage_->getBlockHeader(block_hash));
    if (header.has_value()) {
      return header.value();
    }
    return BlockTreeError::HEADER_NOT_FOUND;
  }

  outcome::result<primitives::BlockHeader> BlockTreeImpl::getBlockHeader(
      const primitives::BlockHash &block_hash) const {
    return block_tree_data_.sharedAccess(
        [&](const BlockTreeData &p)
            -> outcome::result<primitives::BlockHeader> {
          return getBlockHeaderNoLock(p, block_hash);
        });
  }

  outcome::result<primitives::BlockBody> BlockTreeImpl::getBlockBody(
      const primitives::BlockHash &block_hash) const {
    return block_tree_data_.sharedAccess(
        [&](const BlockTreeData &p) -> outcome::result<primitives::BlockBody> {
          OUTCOME_TRY(body, p.storage_->getBlockBody(block_hash));
          if (body.has_value()) {
            return body.value();
          }
          return BlockTreeError::BODY_NOT_FOUND;
        });
  }

  outcome::result<primitives::Justification>
  BlockTreeImpl::getBlockJustification(
      const primitives::BlockHash &block_hash) const {
    return block_tree_data_.sharedAccess(
        [&](const BlockTreeData &p)
            -> outcome::result<primitives::Justification> {
          OUTCOME_TRY(justification, p.storage_->getJustification(block_hash));
          if (justification.has_value()) {
            return justification.value();
          }
          return BlockTreeError::JUSTIFICATION_NOT_FOUND;
        });
  }

  BlockTree::BlockHashVecRes BlockTreeImpl::getBestChainFromBlock(
      const primitives::BlockHash &block, uint64_t maximum) const {
    return block_tree_data_.sharedAccess([&](const BlockTreeData &p)
                                             -> BlockTree::BlockHashVecRes {
      auto block_number_res = p.header_repo_->getNumberByHash(block);
      if (block_number_res.has_error()) {
        log_->error(
            "cannot retrieve block {}: {}", block, block_number_res.error());
        return BlockTreeError::HEADER_NOT_FOUND;
      }
      auto start_block_number = block_number_res.value();

      if (maximum == 1) {
        return std::vector{block};
      }

      auto best_block = p.tree_->best();
      BOOST_ASSERT(best_block != nullptr);
      auto current_depth = best_block->depth;

      if (start_block_number >= current_depth) {
        return std::vector{block};
      }

      auto count =
          std::min<uint64_t>(current_depth - start_block_number + 1, maximum);

      primitives::BlockNumber finish_block_number =
          start_block_number + count - 1;

      auto finish_block_hash_res =
          p.header_repo_->getHashByNumber(finish_block_number);
      if (finish_block_hash_res.has_error()) {
        log_->error("cannot retrieve block with number {}: {}",
                    finish_block_number,
                    finish_block_hash_res.error());
        return BlockTreeError::HEADER_NOT_FOUND;
      }
      const auto &finish_block_hash = finish_block_hash_res.value();

      OUTCOME_TRY(chain,
                  getDescendingChainToBlockNoLock(p, finish_block_hash, count));

      if (chain.back() != block) {
        return std::vector{block};
      }
      std::reverse(chain.begin(), chain.end());
      return chain;
    });
  }

  BlockTree::BlockHashVecRes BlockTreeImpl::getDescendingChainToBlockNoLock(
      const BlockTreeData &p,
      const primitives::BlockHash &to_block,
      uint64_t maximum) const {
    std::vector<primitives::BlockHash> chain;

    auto hash = to_block;

    // Try to retrieve from cached tree
    if (auto node = p.tree_->getRoot().findByHash(hash)) {
      while (maximum > chain.size()) {
        auto parent = node->parent();
        if (not parent) {
          hash = node->block_hash;
          break;
        }
        chain.emplace_back(node->block_hash);
        node = parent;
      }
    }

    while (maximum > chain.size()) {
      auto header_res = p.header_repo_->getBlockHeader(hash);
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

  BlockTree::BlockHashVecRes BlockTreeImpl::getDescendingChainToBlock(
      const primitives::BlockHash &to_block, uint64_t maximum) const {
    return block_tree_data_.sharedAccess([&](const BlockTreeData &p) {
      return getDescendingChainToBlockNoLock(p, to_block, maximum);
    });
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlocks(
      const primitives::BlockHash &ancestor,
      const primitives::BlockHash &descendant) const {
    return block_tree_data_.sharedAccess(
        [&](const BlockTreeData &p) -> BlockTreeImpl::BlockHashVecRes {
          OUTCOME_TRY(from, p.header_repo_->getNumberByHash(ancestor));
          OUTCOME_TRY(to, p.header_repo_->getNumberByHash(descendant));
          if (to < from) {
            return BlockTreeError::TARGET_IS_PAST_MAX;
          }
          auto count = to - from + 1;
          OUTCOME_TRY(chain,
                      getDescendingChainToBlockNoLock(p, descendant, count));
          if (chain.size() != count) {
            return BlockTreeError::EXISTING_BLOCK_NOT_FOUND;
          }
          if (chain.back() != ancestor) {
            return BlockTreeError::BLOCK_ON_DEAD_END;
          }
          std::reverse(chain.begin(), chain.end());
          return chain;
        });
  }

  bool BlockTreeImpl::hasDirectChainNoLock(
      const BlockTreeData &p,
      const primitives::BlockHash &ancestor,
      const primitives::BlockHash &descendant) const {
    auto ancestor_node_ptr = p.tree_->getRoot().findByHash(ancestor);
    auto descendant_node_ptr = p.tree_->getRoot().findByHash(descendant);

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
      auto number_res = p.header_repo_->getNumberByHash(ancestor);
      if (!number_res) {
        return false;
      }
      ancestor_depth = number_res.value();
    }
    if (descendant_node_ptr) {
      descendant_depth = descendant_node_ptr->depth;
    } else {
      auto number_res = p.header_repo_->getNumberByHash(descendant);
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
      return canDescend(descendant_node_ptr, ancestor_node_ptr);
    }

    // else, we need to use a database

    // Try to use optimal way, if ancestor and descendant in the finalized
    // chain
    if (descendant_depth <= getLastFinalizedNoLock(p).number) {
      auto res = p.header_repo_->getHashByNumber(descendant_depth);
      BOOST_ASSERT_MSG(res.has_value(),
                       "Any finalized block must be accessible by number");
      // Check if descendant in finalised chain
      if (res.value() == descendant) {
        res = p.header_repo_->getHashByNumber(ancestor_depth);
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
      auto current_header_res = p.header_repo_->getBlockHeader(current_hash);
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

  bool BlockTreeImpl::hasDirectChain(
      const primitives::BlockHash &ancestor,
      const primitives::BlockHash &descendant) const {
    return block_tree_data_.sharedAccess([&](const BlockTreeData &p) {
      return hasDirectChainNoLock(p, ancestor, descendant);
    });
  }

  bool BlockTreeImpl::isFinalized(const primitives::BlockInfo &block) const {
    return block_tree_data_.sharedAccess([&](const BlockTreeData &p) {
      return block.number <= getLastFinalizedNoLock(p).number
         and p.header_repo_->getHashByNumber(block.number)
                 == outcome::success(block.hash);
    });
  }

  primitives::BlockInfo BlockTreeImpl::bestBlockNoLock(
      const BlockTreeData &p) const {
    return p.tree_->best()->getBlockInfo();
  }

  primitives::BlockInfo BlockTreeImpl::bestBlock() const {
    return block_tree_data_.sharedAccess(
        [&](const BlockTreeData &p) { return bestBlockNoLock(p); });
  }

  outcome::result<primitives::BlockInfo> BlockTreeImpl::getBestContaining(
      const primitives::BlockHash &target_hash) const {
    return block_tree_data_.sharedAccess(
        [&](const BlockTreeData &p) -> outcome::result<primitives::BlockInfo> {
          if (getLastFinalizedNoLock(p).hash == target_hash) {
            return bestBlockNoLock(p);
          }
          auto &root = p.tree_->getRoot();

          auto target = root.findByHash(target_hash);

          // If target has not found in block tree (in memory),
          // it means block finalized or discarded
          if (not target) {
            OUTCOME_TRY(target_number,
                        p.header_repo_->getNumberByHash(target_hash));

            OUTCOME_TRY(canon_hash,
                        p.header_repo_->getHashByNumber(target_number));

            if (canon_hash != target_hash) {
              return BlockTreeError::BLOCK_ON_DEAD_END;
            }

            return bestBlockNoLock(p);
          }

          return p.tree_->bestWith(target);
        });
  }

  std::vector<primitives::BlockHash> BlockTreeImpl::getLeavesNoLock(
      const BlockTreeData &p) const {
    return p.tree_->leafHashes();
  }

  std::vector<primitives::BlockHash> BlockTreeImpl::getLeaves() const {
    return block_tree_data_.sharedAccess(
        [&](const BlockTreeData &p) { return getLeavesNoLock(p); });
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChildren(
      const primitives::BlockHash &block) const {
    return block_tree_data_.sharedAccess([&](const BlockTreeData &p)
                                             -> BlockTreeImpl::BlockHashVecRes {
      if (auto node = p.tree_->getRoot().findByHash(block); node != nullptr) {
        std::vector<primitives::BlockHash> result;
        result.reserve(node->children.size());
        for (const auto &child : node->children) {
          result.push_back(child->block_hash);
        }
        return result;
      }
      OUTCOME_TRY(header, p.storage_->getBlockHeader(block));
      if (!header.has_value()) {
        return BlockTreeError::HEADER_NOT_FOUND;
      }
      // if node is not in tree_ it must be finalized and thus have only one
      // child
      OUTCOME_TRY(child_hash,
                  p.header_repo_->getHashByNumber(header.value().number + 1));
      return outcome::success(std::vector<primitives::BlockHash>{child_hash});
    });
  }

  primitives::BlockInfo BlockTreeImpl::getLastFinalizedNoLock(
      const BlockTreeData &p) const {
    return p.tree_->finalized()->getBlockInfo();
  }

  primitives::BlockInfo BlockTreeImpl::getLastFinalized() const {
    return block_tree_data_.sharedAccess(
        [&](const BlockTreeData &p) { return getLastFinalizedNoLock(p); });
  }

  outcome::result<void> BlockTreeImpl::pruneNoLock(
      BlockTreeData &p, const std::shared_ptr<TreeNode> &lastFinalizedNode) {
    std::deque<std::shared_ptr<TreeNode>> to_remove;

    auto following_node = lastFinalizedNode;

    for (auto current_node = following_node->parent(); current_node;
         current_node = current_node->parent()) {
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
    std::vector<primitives::BlockHash> retired_hashes;

    // remove from storage
    retired_hashes.reserve(to_remove.size());
    for (const auto &node : to_remove) {
      OUTCOME_TRY(block_header_opt,
                  p.storage_->getBlockHeader(node->block_hash));
      OUTCOME_TRY(block_body_opt, p.storage_->getBlockBody(node->block_hash));
      if (block_body_opt.has_value()) {
        extrinsics.reserve(extrinsics.size() + block_body_opt.value().size());
        for (auto &ext : block_body_opt.value()) {
          auto extrinsic_hash = p.hasher_->blake2b_256(ext.data);
          if (auto key = p.extrinsic_event_key_repo_->get(extrinsic_hash)) {
            main_thread_.execute([wself{weak_from_this()},
                                  key{key.value()},
                                  block_hash{node->block_hash}]() {
              if (auto self = wself.lock()) {
                self->extrinsic_events_engine_->notify(
                    key,
                    primitives::events::ExtrinsicLifecycleEvent::Retracted(
                        key, block_hash));
              }
            });
          }
          extrinsics.emplace_back(std::move(ext));
        }
        BOOST_ASSERT(block_header_opt.has_value());
        OUTCOME_TRY(p.state_pruner_->pruneDiscarded(block_header_opt.value()));
      }

      retired_hashes.emplace_back(node->block_hash);
      p.tree_->removeFromMeta(node);
      OUTCOME_TRY(p.storage_->removeBlock(node->block_hash));
    }

    auto last_finalized_block_info = getLastFinalizedNoLock(p);
    for (primitives::BlockNumber n = last_finalized_block_info.number;
         n < lastFinalizedNode->getBlockInfo().number;
         ++n) {
      if (auto result = p.storage_->getBlockHash(n);
          result.has_value() && result.value()) {
        retired_hashes.emplace_back(std::move(*result.value()));
      }
    }

    // trying to return extrinsics back to transaction pool
    main_thread_.execute([extrinsics{std::move(extrinsics)},
                          wself{weak_from_this()},
                          retired_hashes{std::move(retired_hashes)}]() mutable {
      if (auto self = wself.lock()) {
        auto eo = self->block_tree_data_.sharedAccess(
            [&](const BlockTreeData &p) { return p.extrinsic_observer_; });

        for (auto &&extrinsic : extrinsics) {
          auto result = eo->onTxMessage(extrinsic);
          if (result) {
            SL_DEBUG(self->log_, "Tx {} was reapplied", result.value().toHex());
          } else {
            SL_DEBUG(self->log_, "Tx was skipped: {}", result.error());
          }
        }

        self->chain_events_engine_->notify(
            primitives::events::ChainEventType::kDeactivateAfterFinalization,
            retired_hashes);
      }
    });

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::pruneTrie(
      const BlockTreeData &block_tree_data,
      primitives::BlockNumber new_finalized) {
    // pruning is disabled
    if (!block_tree_data.state_pruner_->getPruningDepth().has_value()) {
      return outcome::success();
    }
    auto last_pruned = block_tree_data.state_pruner_->getLastPrunedBlock();

    BOOST_ASSERT(!last_pruned.has_value()
                 || last_pruned.value().number
                        <= block_tree_data.tree_->getRoot().depth);
    auto next_pruned_number = last_pruned ? last_pruned->number + 1 : 0;

    OUTCOME_TRY(hash_opt, getBlockHash(next_pruned_number));
    BOOST_ASSERT(hash_opt.has_value());
    primitives::BlockHash hash = std::move(*hash_opt);
    auto pruning_depth =
        block_tree_data.state_pruner_->getPruningDepth().value_or(0);
    if (new_finalized < pruning_depth) {
      return outcome::success();
    }

    auto last_to_prune = new_finalized - pruning_depth;
    for (auto n = next_pruned_number; n < last_to_prune; n++) {
      OUTCOME_TRY(next_hash_opt, getBlockHash(n + 1));
      BOOST_ASSERT(next_hash_opt.has_value());
      auto &next_hash = *next_hash_opt;
      OUTCOME_TRY(header, getBlockHeader(hash));
      OUTCOME_TRY(block_tree_data.state_pruner_->pruneFinalized(header));
      hash = std::move(next_hash);
    }

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::reorganizeNoLock(BlockTreeData &p) {
    auto block = BlockTreeImpl::bestBlockNoLock(p);

    // Remove assigning of obsolete best upper blocks chain
    auto prev_max_best_block_number = block.number;
    for (;;) {
      auto hash_res =
          p.header_repo_->getHashByNumber(prev_max_best_block_number + 1);
      if (hash_res.has_error()) {
        if (hash_res == outcome::failure(BlockTreeError::HEADER_NOT_FOUND)) {
          break;
        }
        return hash_res.as_failure();
      }
      ++prev_max_best_block_number;
    }
    for (auto number = prev_max_best_block_number; number > block.number;
         --number) {
      OUTCOME_TRY(p.storage_->deassignNumberToHash(number));
    }

    auto hash_res = p.header_repo_->getHashByNumber(block.number);
    if (hash_res.has_error()) {
      if (hash_res != outcome::failure(BlockTreeError::HEADER_NOT_FOUND)) {
        return hash_res.as_failure();
      }
    } else if (block.hash == hash_res.value()) {
      return outcome::success();
    }

    // Rewrite earlier blocks sequence
    size_t count = 0;
    for (;;) {
      OUTCOME_TRY(p.storage_->assignNumberToHash(block));
      if (block.number == 0) {
        break;
      }
      OUTCOME_TRY(header, getBlockHeaderNoLock(p, block.hash));
      auto parent_hash_res = p.header_repo_->getHashByNumber(block.number - 1);
      if (parent_hash_res.has_error()) {
        if (parent_hash_res
            != outcome::failure(BlockTreeError::HEADER_NOT_FOUND)) {
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

  void BlockTreeImpl::warp(const primitives::BlockInfo &block_info) {
    block_tree_data_.exclusiveAccess([&](BlockTreeData &p) {
      p.tree_ = std::make_unique<CachedTree>(block_info);
      metric_known_chain_leaves_->set(1);
      metric_best_block_height_->set(block_info.number);
      telemetry_->notifyBlockFinalized(block_info);
      telemetry_->pushBlockStats();
      metric_finalized_block_height_->set(block_info.number);
    });
  }

  void BlockTreeImpl::notifyBestAndFinalized() {
    auto best_info = bestBlock();
    auto best_header = getBlockHeader(best_info.hash).value();
    chain_events_engine_->notify(primitives::events::ChainEventType::kNewHeads,
                                 best_header);
    auto finalized_info = getLastFinalized();
    auto finalized_header = getBlockHeader(finalized_info.hash).value();
    chain_events_engine_->notify(
        primitives::events::ChainEventType::kFinalizedHeads, finalized_header);
  }

  void BlockTreeImpl::removeUnfinalized() {
    auto r = block_tree_data_.exclusiveAccess(
        [&](BlockTreeData &p) -> outcome::result<void> {
          auto finalized = p.tree_->getRoot().getBlockInfo();
          auto nodes = std::move(p.tree_->getRoot().children);
          primitives::BlockNumber max = 0;
          for (size_t i = 0; i < nodes.size(); ++i) {
            auto children = std::move(nodes[i]->children);
            for (auto &node : children) {
              max = std::max(max, node->depth);
              nodes.emplace_back(std::move(node));
            }
          }
          p.tree_ = std::make_unique<CachedTree>(finalized);
          OUTCOME_TRY(p.storage_->setBlockTreeLeaves({finalized.hash}));
          for (auto i = max; i > finalized.number; --i) {
            OUTCOME_TRY(p.storage_->deassignNumberToHash(i));
          }
          std::reverse(nodes.begin(), nodes.end());
          for (auto &node : nodes) {
            OUTCOME_TRY(p.storage_->removeBlock(node->block_hash));
          }
          return outcome::success();
        });
    if (not r) {
      SL_ERROR(log_, "removeUnfinalized error: {}", r.error());
    }
  }
}  // namespace kagome::blockchain
