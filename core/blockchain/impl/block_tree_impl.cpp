/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_tree_impl.hpp"

#include <algorithm>
#include <stack>

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/cached_tree.hpp"
#include "blockchain/impl/common.hpp"
#include "blockchain/impl/justification_storage_policy.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
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

        BOOST_ASSERT_MSG(not block_tree_unordered_leaves.empty(),
                         "Must be known or calculated at least one leaf");

        for (auto &hash : block_tree_unordered_leaves) {
          auto res = header_repo->getNumberById(hash);
          if (res.has_error()) {
            if (res
                == outcome::failure(
                    blockchain::BlockTreeError::EXISTING_BLOCK_NOT_FOUND)) {
              SL_TRACE(log, "Leaf {} not found", hash);
              continue;
            }
            SL_ERROR(
                log, "Leaf {} is corrupted: {}", hash, res.error().message());
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
          number = lower + (upper - lower) / 2;

          auto res = storage->hasBlockHeader(number);
          if (res.has_failure()) {
            SL_CRITICAL(
                log, "Search best block has failed: {}", res.error().message());
            return BlockTreeError::HEADER_NOT_FOUND;
          }

          if (res.value()) {
            SL_TRACE(log, "bisect {} -> found", number);
            lower = number;
          } else {
            SL_TRACE(log, "bisect {} -> not found", number);
            upper = number - 1;
          }
          if (lower + 1 == upper) {
            number = lower;
            break;
          }
        }

        OUTCOME_TRY(hash, header_repo->getHashById(number));
        block_tree_leaves.emplace(number, hash);

        if (auto res = storage->setBlockTreeLeaves({hash}); res.has_error()) {
          SL_CRITICAL(log,
                      "Can't save recovered block tree leaves: {}",
                      res.error().message());
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
      std::shared_ptr<runtime::Core> runtime_core,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
      std::shared_ptr<consensus::BabeUtil> babe_util,
      std::shared_ptr<const class JustificationStoragePolicy>
          justification_storage_policy) {
    BOOST_ASSERT(storage != nullptr);
    BOOST_ASSERT(header_repo != nullptr);

    log::Logger log = log::createLogger("BlockTree", "blockchain");

    OUTCOME_TRY(block_tree_leaves, loadLeaves(storage, header_repo, log));

    BOOST_ASSERT_MSG(not block_tree_leaves.empty(),
                     "Must be known or calculated at least one leaf");

    // Find the least and best leaf
    auto least_leaf = *block_tree_leaves.begin();
    auto best_leaf = *block_tree_leaves.rbegin();

    OUTCOME_TRY(last_finalized_block_info, storage->getLastFinalized());

    std::optional<consensus::EpochNumber> curr_epoch_number;

    // First, look up slot number of block number 1
    OUTCOME_TRY(first_block_header_exists, storage->hasBlockHeader(1));
    if (first_block_header_exists) {
      OUTCOME_TRY(first_block_header, storage->getBlockHeader(1));
      BOOST_ASSERT_MSG(first_block_header.has_value(),
                       "Existence of this header has been checked");
      auto babe_digest_res =
          consensus::getBabeDigests(first_block_header.value());
      BOOST_ASSERT_MSG(babe_digest_res.has_value(),
                       "Any non genesis block must contain babe digest");
      auto first_slot_number = babe_digest_res.value().second.slot_number;

      // Second, look up slot number of best block
      auto &best_block_hash = best_leaf.hash;
      OUTCOME_TRY(best_block_header_opt,
                  storage->getBlockHeader(best_block_hash));
      BOOST_ASSERT_MSG(best_block_header_opt.has_value(),
                       "Best block must be known whenever");
      const auto &best_block_header = best_block_header_opt.value();
      babe_digest_res = consensus::getBabeDigests(best_block_header);
      BOOST_ASSERT_MSG(babe_digest_res.has_value(),
                       "Any non genesis block must contain babe digest");
      auto last_slot_number = babe_digest_res.value().second.slot_number;

      BOOST_ASSERT_MSG(
          last_slot_number >= first_slot_number,
          "Non genesis slot must not be less then slot of block number 1");

      // Now we have all to calculate epoch number
      auto epoch_number = (last_slot_number - first_slot_number)
                          / babe_configuration->epoch_length;

      babe_util->syncEpoch([&] {
        auto is_first_block_finalized = last_finalized_block_info.number > 0;
        return std::tuple(first_slot_number, is_first_block_finalized);
      });

      curr_epoch_number.emplace(epoch_number);
    }

    OUTCOME_TRY(last_finalized_justification,
                storage->getJustification(last_finalized_block_info.hash));

    std::optional<consensus::EpochDigest> curr_epoch;
    std::optional<consensus::EpochDigest> next_epoch;
    auto hash_tmp = last_finalized_block_info.hash;

    // We are going block by block to genesis direction and observes them for
    // find epoch digest. First found digest if it in the block assigned to the
    // current epoch will be saved as digest planned for next epoch. First found
    // digest if it in the block assigned to the early epoch will be saved as
    // digest for current epoch (and planned for next epoch,
    // if it is yet undefined).

    for (;;) {
      if (hash_tmp == primitives::BlockHash{}) {
        if (not curr_epoch_number.has_value()) {
          curr_epoch_number = 0;
        }
        if (not curr_epoch.has_value()) {
          curr_epoch.emplace(consensus::EpochDigest{
              .authorities = babe_configuration->genesis_authorities,
              .randomness = babe_configuration->randomness});
          SL_TRACE(log,
                   "Current epoch data has got basing genesis: "
                   "Epoch #{}, Randomness: {}",
                   curr_epoch_number.value(),
                   curr_epoch.value().randomness);
        }
        if (not next_epoch.has_value()) {
          next_epoch.emplace(consensus::EpochDigest{
              .authorities = babe_configuration->genesis_authorities,
              .randomness = babe_configuration->randomness});
          SL_TRACE(log,
                   "Next epoch data has got basing genesis: "
                   "Epoch #1+, Randomness: {}",
                   next_epoch.value().randomness);
        }
        break;
      }

      OUTCOME_TRY(header_opt_tmp, storage->getBlockHeader(hash_tmp));
      BOOST_ASSERT_MSG(
          header_opt_tmp.has_value(),
          "Header retrieved by hash previously retrieved from "
          "HeaderRepository, thus the header should be present there");
      auto &header_tmp = header_opt_tmp.value();

      auto babe_digests_res = consensus::getBabeDigests(header_tmp);
      if (not babe_digests_res) {
        hash_tmp = header_tmp.parent_hash;
        continue;
      }

      auto slot_number = babe_digests_res.value().second.slot_number;
      auto epoch_number = babe_util->slotToEpoch(slot_number);

      if (not curr_epoch_number.has_value()) {
        curr_epoch_number = epoch_number;
        SL_TRACE(log,
                 "Current epoch number has gotten by slot number: {}",
                 curr_epoch_number.value());
      }

      if (auto digest = consensus::getNextEpochDigest(header_tmp);
          digest.has_value()) {
        SL_TRACE(log,
                 "Epoch digest found in block {} (slot {}, epoch {}). "
                 "Randomness: {}",
                 primitives::BlockInfo(header_tmp.number, hash_tmp),
                 slot_number,
                 epoch_number,
                 digest.value().randomness);

        if (not next_epoch.has_value()) {
          next_epoch.emplace(digest.value());
          SL_TRACE(log,
                   "Next epoch data is set by obtained digest: "
                   "Epoch #{}+, Randomness: {}",
                   epoch_number + 1,
                   next_epoch.value().randomness);
        }
        if (epoch_number != curr_epoch_number) {
          curr_epoch.emplace(digest.value());
          SL_TRACE(log,
                   "Current epoch data is set by obtained digest: "
                   "Epoch #{}, Randomness: {}",
                   curr_epoch_number.value(),
                   curr_epoch.value().randomness);
          break;
        }
      }

      hash_tmp = header_tmp.parent_hash;
    }

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
                    header_res.error().message());
            break;
          }
          auto &header_opt = header_res.value();
          if (!header_opt.has_value()) {
            SL_WARN(log,
                    "Can't get header of existing block {}: not found in block "
                    "storage",
                    hash,
                    header_res.error().message());
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
                                           std::move(curr_epoch.value()),
                                           curr_epoch_number.value(),
                                           std::move(next_epoch.value()),
                                           true);
    SL_DEBUG(log, "Last finalized block #{}", tree->depth);
    auto meta = std::make_shared<TreeMeta>(tree, last_finalized_justification);

    auto *block_tree =
        new BlockTreeImpl(std::move(header_repo),
                          std::move(storage),
                          std::make_unique<CachedTree>(tree, meta),
                          std::move(extrinsic_observer),
                          std::move(hasher),
                          std::move(chain_events_engine),
                          std::move(extrinsic_events_engine),
                          std::move(extrinsic_event_key_repo),
                          std::move(runtime_core),
                          std::move(changes_tracker),
                          std::move(babe_util),
                          std::move(justification_storage_policy));

    // Add non-finalized block to the block tree
    for (auto &e : collected) {
      const auto &block = e.first;
      const auto header = std::move(e.second);

      auto res = block_tree->addExistingBlock(block.hash, header);
      if (res.has_error()) {
        SL_WARN(log,
                "Can't add existing non-finalized block {} to block tree: {}",
                block,
                res.error().message());
      }
      SL_TRACE(
          log, "Existing non-finalized block {} is added to block tree", block);
    }

    return std::shared_ptr<BlockTreeImpl>(block_tree);
  }

  outcome::result<void> BlockTreeImpl::recover(
      primitives::BlockId target_block,
      std::shared_ptr<BlockStorage> storage,
      std::shared_ptr<BlockHeaderRepository> header_repo,
      std::shared_ptr<const storage::trie::TrieStorage> trie_storage) {
    BOOST_ASSERT(storage != nullptr);
    BOOST_ASSERT(header_repo != nullptr);
    BOOST_ASSERT(trie_storage != nullptr);

    log::Logger log = log::createLogger("BlockTree", "blockchain");

    OUTCOME_TRY(block_tree_leaves, loadLeaves(storage, header_repo, log));

    BOOST_ASSERT_MSG(not block_tree_leaves.empty(),
                     "Must be known or calculated at least one leaf");

    // Check if target block exists
    auto target_block_header_opt_res = storage->getBlockHeader(target_block);
    if (target_block_header_opt_res.has_error()) {
      SL_CRITICAL(log,
                  "Can't get header of target block: {}",
                  target_block_header_opt_res.error().message());
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
      SL_WARN(
          log, "Can't get state of target block: {}", res.error().message());
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
                    header_opt_res.error().message());
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
        SL_CRITICAL(log,
                    "Can't save updated block tree leaves: {}",
                    res.error().message());
        return res.as_failure();
      }

      if (auto res = storage->removeBlock(block); res.has_error()) {
        SL_CRITICAL(
            log, "Can't remove block {}: {}", block, res.error().message());
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
      std::shared_ptr<runtime::Core> runtime_core,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<consensus::BabeUtil> babe_util,
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
        runtime_core_(std::move(runtime_core)),
        trie_changes_tracker_(std::move(changes_tracker)),
        babe_util_(std::move(babe_util)),
        justification_storage_policy_{std::move(justification_storage_policy)} {
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(tree_ != nullptr);
    BOOST_ASSERT(extrinsic_observer_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(chain_events_engine_ != nullptr);
    BOOST_ASSERT(extrinsic_events_engine_ != nullptr);
    BOOST_ASSERT(extrinsic_event_key_repo_ != nullptr);
    BOOST_ASSERT(runtime_core_ != nullptr);
    BOOST_ASSERT(trie_changes_tracker_ != nullptr);
    BOOST_ASSERT(babe_util_ != nullptr);
    BOOST_ASSERT(justification_storage_policy_ != nullptr);
    BOOST_ASSERT(telemetry_ != nullptr);

    // Register metrics
    metrics_registry_->registerGaugeFamily(blockHeightMetricName,
                                           "Block height info of the chain");

    metric_best_block_height_ = metrics_registry_->registerGaugeMetric(
        blockHeightMetricName, {{"status", "best"}});
    metric_best_block_height_->set(
        tree_->getMetadata().deepest_leaf.lock()->depth);

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

    consensus::EpochNumber epoch_number = 0;
    auto babe_digests_res = consensus::getBabeDigests(header);
    if (babe_digests_res.has_value()) {
      epoch_number =
          babe_util_->slotToEpoch(babe_digests_res.value().second.slot_number);
    }

    std::optional<consensus::EpochDigest> next_epoch;
    if (auto digest = consensus::getNextEpochDigest(header);
        digest.has_value()) {
      next_epoch.emplace(std::move(digest.value()));
    }

    // update local meta with the new block
    auto new_node = std::make_shared<TreeNode>(
        block_hash, header.number, parent, epoch_number, std::move(next_epoch));

    tree_->updateMeta(new_node);

    OUTCOME_TRY(reorganize());

    OUTCOME_TRY(
        storage_->setBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
                                      tree_->getMetadata().leaves.end()}));

    metric_known_chain_leaves_->set(tree_->getMetadata().leaves.size());
    metric_best_block_height_->set(
        tree_->getMetadata().deepest_leaf.lock()->depth);

    chain_events_engine_->notify(primitives::events::ChainEventType::kNewHeads,
                                 header);

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

    consensus::EpochNumber epoch_number = 0;
    auto babe_digests_res = consensus::getBabeDigests(block.header);
    if (babe_digests_res.has_value()) {
      auto babe_slot = babe_digests_res.value().second.slot_number;
      epoch_number = babe_util_->slotToEpoch(babe_slot);
    }

    std::optional<consensus::EpochDigest> next_epoch;
    if (auto digest = consensus::getNextEpochDigest(block.header);
        digest.has_value()) {
      next_epoch.emplace(std::move(digest.value()));
    }

    // Update local meta with the block
    auto new_node = std::make_shared<TreeNode>(block_hash,
                                               block.header.number,
                                               parent,
                                               epoch_number,
                                               std::move(next_epoch));

    tree_->updateMeta(new_node);

    OUTCOME_TRY(reorganize());

    OUTCOME_TRY(
        storage_->setBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
                                      tree_->getMetadata().leaves.end()}));

    chain_events_engine_->notify(primitives::events::ChainEventType::kNewHeads,
                                 block.header);
    trie_changes_tracker_->onBlockAdded(block_hash);
    for (const auto &ext : block.body) {
      if (auto key =
              extrinsic_event_key_repo_->get(hasher_->blake2b_256(ext.data))) {
        extrinsic_events_engine_->notify(
            key.value(),
            primitives::events::ExtrinsicLifecycleEvent::InBlock(key.value(),
                                                                 block_hash));
      }
    }

    metric_known_chain_leaves_->set(tree_->getMetadata().leaves.size());
    metric_best_block_height_->set(
        tree_->getMetadata().deepest_leaf.lock()->depth);

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

    // Remove from block tree
    tree_->removeFromMeta(node);

    OUTCOME_TRY(reorganize());

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

    consensus::EpochNumber epoch_number = 0;
    auto babe_digests_res = consensus::getBabeDigests(block_header);
    if (babe_digests_res.has_value()) {
      auto babe_slot = babe_digests_res.value().second.slot_number;
      epoch_number = babe_util_->slotToEpoch(babe_slot);
    }

    std::optional<consensus::EpochDigest> next_epoch;
    if (auto digest = consensus::getNextEpochDigest(block_header);
        digest.has_value()) {
      next_epoch.emplace(std::move(digest.value()));
    }

    // Update local meta with the block
    auto new_node = std::make_shared<TreeNode>(block_hash,
                                               block_header.number,
                                               parent,
                                               epoch_number,
                                               std::move(next_epoch));

    tree_->updateMeta(new_node);

    OUTCOME_TRY(reorganize());

    OUTCOME_TRY(
        storage_->setBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
                                      tree_->getMetadata().leaves.end()}));

    metric_known_chain_leaves_->set(tree_->getMetadata().leaves.size());
    metric_best_block_height_->set(
        tree_->getMetadata().deepest_leaf.lock()->depth);

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

    // it has failure result when fast sync is in progress
    auto new_runtime_version = runtime_core_->version(block_hash);
    if (new_runtime_version.has_value()) {
      if (not actual_runtime_version_.has_value()
          || actual_runtime_version_ != new_runtime_version.value()) {
        actual_runtime_version_ = new_runtime_version.value();
        chain_events_engine_->notify(
            primitives::events::ChainEventType::kFinalizedRuntimeVersion,
            new_runtime_version.value());
      }
    }

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
      log_->error("cannot retrieve block with hash {}: {}",
                  block.toHex(),
                  block_number_res.error().message());
      return BlockTreeError::HEADER_NOT_FOUND;
    }
    auto start_block_number = block_number_res.value();

    auto deepest_leaf = tree_->getMetadata().deepest_leaf.lock();
    BOOST_ASSERT(deepest_leaf != nullptr);
    auto current_depth = deepest_leaf->depth;

    primitives::BlockNumber finish_block_number =
        start_block_number + maximum - 1;
    if (current_depth < finish_block_number) {
      finish_block_number = current_depth;
    }

    auto finish_block_hash_res =
        header_repo_->getHashByNumber(finish_block_number);
    if (finish_block_hash_res.has_error()) {
      log_->error("cannot retrieve block with number {}: {}",
                  finish_block_number,
                  finish_block_hash_res.error().message());
      return BlockTreeError::HEADER_NOT_FOUND;
    }
    const auto &finish_block_hash = finish_block_hash_res.value();

    return getChainByBlocks(block, finish_block_hash, maximum);
  }

  BlockTree::BlockHashVecRes BlockTreeImpl::getDescendingChainToBlock(
      const primitives::BlockHash &to_block, uint64_t maximum) const {
    std::deque<primitives::BlockHash> chain;

    auto hash = to_block;

    // Try to retrieve from cached tree
    if (auto node = tree_->getRoot().findByHash(hash)) {
      chain.emplace_back(hash);
      while (maximum > chain.size()) {
        auto parent = node->parent.lock();
        if (not parent) {
          hash = node->block_hash;
          break;
        }
        chain.emplace_back(parent->block_hash);
        node = parent;
      }
    }

    while (maximum > chain.size()) {
      auto header_res = header_repo_->getBlockHeader(hash);
      if (header_res.has_error()) {
        if (chain.empty()) {
          log_->error("cannot retrieve block with hash {}: {}",
                      hash,
                      header_res.error().message());
          return BlockTreeError::HEADER_NOT_FOUND;
        }
        break;
      }
      const auto &header = header_res.value();

      chain.emplace_back(hash);

      if (header.parent_hash == primitives::BlockHash{}) {
        break;
      }

      hash = header.parent_hash;
    }

    return std::vector<primitives::BlockHash>(chain.begin(), chain.end());
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlocks(
      const primitives::BlockHash &top_block,
      const primitives::BlockHash &bottom_block,
      const uint32_t max_count) const {
    return getChainByBlocks(
        top_block, bottom_block, std::make_optional(max_count));
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlocks(
      const primitives::BlockHash &top_block,
      const primitives::BlockHash &bottom_block,
      std::optional<uint32_t> max_count) const {
    OUTCOME_TRY(from, header_repo_->getNumberByHash(top_block));
    OUTCOME_TRY(to, header_repo_->getNumberByHash(bottom_block));

    if (auto chain_res = tryGetChainByBlocksFromCache(
            primitives::BlockInfo{from, top_block},
            primitives::BlockInfo{to, bottom_block},
            max_count)) {
      return chain_res.value();
    }

    std::vector<primitives::BlockHash> result;
    if (to < from) {
      return result;
    }

    const auto response_length =
        max_count ? std::min(to - from + 1, max_count.value())
                  : (to - from + 1);
    result.reserve(response_length);

    SL_TRACE(log_,
             "Try to create {} length chain from number {} to {}.",
             response_length,
             from,
             to);

    auto current_hash = bottom_block;

    std::deque<primitives::BlockHash> chain;
    chain.emplace_back(current_hash);
    size_t count = 0;
    while (current_hash != top_block && result.size() < response_length) {
      if (max_count.has_value() && ++count > max_count.value()) {
        log_->warn(
            "impossible to get chain by blocks: "
            "max count exceeded at intermediate block {}",
            current_hash);
        break;
      }
      auto header_res = header_repo_->getBlockHeader(current_hash);
      if (!header_res) {
        log_->warn(
            "impossible to get chain by blocks: "
            "intermediate block {} was not added to block tree before",
            current_hash);
        return BlockTreeError::SOME_BLOCK_IN_CHAIN_NOT_FOUND;
      }
      current_hash = header_res.value().parent_hash;
      if (chain.size() >= response_length) {
        chain.pop_front();
      }
      chain.emplace_back(current_hash);
    }

    result.assign(chain.crbegin(), chain.crend());
    return result;
  }

  std::optional<std::vector<primitives::BlockHash>>
  BlockTreeImpl::tryGetChainByBlocksFromCache(
      const primitives::BlockInfo &top_block,
      const primitives::BlockInfo &bottom_block,
      std::optional<uint32_t> max_count) const {
    if (auto from = tree_->getRoot().findByHash(top_block.hash)) {
      if (bottom_block.number < from->depth) {
        return std::nullopt;
      }
      const auto in_tree_branch_len = bottom_block.number - from->depth + 1;
      const auto response_length =
          max_count ? std::min(in_tree_branch_len, max_count.value())
                    : in_tree_branch_len;

      std::vector<primitives::BlockHash> result;
      result.reserve(response_length);

      auto res = from->applyToChain(
          bottom_block,
          [&result, response_length](auto &node) -> TreeNode::ExitToken {
            result.emplace_back(node.block_hash);
            if (result.size() == response_length) {
              return TreeNode::ExitToken::EXIT;
            }
            return TreeNode::ExitToken::CONTINUE;
          });
      if (res.has_error()) {
        SL_DEBUG(log_,
                 "Failed to collect a chain of blocks from {} to {}: {}",
                 top_block,
                 bottom_block,
                 res.error().message());
        return std::nullopt;
      }
      SL_TRACE(log_,
               "Create {} length chain from number {} to {} from cache.",
               response_length,
               from->depth,
               bottom_block.number);

      return result;
    }
    return std::nullopt;
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlocks(
      const primitives::BlockHash &top_block,
      const primitives::BlockHash &bottom_block) const {
    return getChainByBlocks(top_block, bottom_block, std::nullopt);
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
      SL_WARN(log_,
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

  primitives::BlockInfo BlockTreeImpl::deepestLeaf() const {
    auto &&leaf = tree_->getMetadata().deepest_leaf.lock();
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
        target_hash.toHex(),
        max_number.has_value() ? max_number.value() : -1);
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

  outcome::result<consensus::EpochDigest> BlockTreeImpl::getEpochDigest(
      consensus::EpochNumber epoch_number,
      primitives::BlockHash block_hash) const {
    auto node = tree_->getRoot().findByHash(block_hash);
    if (node) {
      if (node->epoch_number != epoch_number) {
        return *node->next_epoch_digest;
      }
      return *node->epoch_digest;
    }
    return BlockTreeError::NON_FINALIZED_BLOCK_NOT_FOUND;
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
        SL_DEBUG(log_, "Tx was skipped: {}", result.error().message());
      }
    }

    return outcome::success();
  }

  outcome::result<void> BlockTreeImpl::reorganize() {
    auto block = BlockTreeImpl::deepestLeaf();
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
