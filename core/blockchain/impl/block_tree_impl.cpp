/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_tree_impl.hpp"

#include <algorithm>

#include "blockchain/block_storage_error.hpp"
#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/cached_tree.hpp"
#include "blockchain/impl/common.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "crypto/blake2/blake2b.h"
#include "storage/changes_trie/changes_tracker.hpp"
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

namespace {
  constexpr auto blockHeightMetricName = "kagome_block_height";
  constexpr auto knownChainLeavesMetricName = "kagome_known_chain_leaves";
}  // namespace

namespace kagome::blockchain {
  using Buffer = common::Buffer;
  using Prefix = prefix::Prefix;
  using DatabaseError = kagome::storage::DatabaseError;

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
      std::shared_ptr<consensus::BabeUtil> babe_util) {
    BOOST_ASSERT(storage != nullptr);

    log::Logger log = log::createLogger("BlockTree", "blockchain");

    std::vector<primitives::BlockHash> block_tree_leaves;
    auto block_tree_leaves_res = storage->loadBlockTreeLeaves();
    if (block_tree_leaves_res.has_value()) {
      std::swap(block_tree_leaves, block_tree_leaves_res.value());
    } else if (block_tree_leaves_res
               == outcome::failure(
                   BlockStorageError::BLOCK_TREE_LEAVES_NOT_FOUND)) {
      // Fallback way to get last finalized
      // TODO(xDimon): After deploy of this change,
      //  getting of finalized block from storage should be removed
      auto last_finalized_block_res = storage->getLastFinalizedBlockHash();
      if (last_finalized_block_res.has_value()) {
        auto &last_finalized_block = last_finalized_block_res.value();
        block_tree_leaves.emplace_back(std::move(last_finalized_block));
      } else {
        OUTCOME_TRY(genesis_hash, storage->getGenesisBlockHash());
        block_tree_leaves.emplace_back(std::move(genesis_hash));
      }
    } else {
      return block_tree_leaves_res.as_failure();
    }

    BOOST_ASSERT_MSG(not block_tree_leaves.empty(),
                     "Must be known or calculated at least one leaf");

    // Find the least leaf
    primitives::BlockInfo least_leaf(
        std::numeric_limits<primitives::BlockNumber>::max(), {});
    for (auto hash : block_tree_leaves) {
      OUTCOME_TRY(number, header_repo->getNumberById(hash));
      if (number < least_leaf.number) {
        least_leaf = {number, hash};
      }
    }

    primitives::BlockHash last_finalized_block;

    // Backward search of finalized block
    for (auto block_info = least_leaf;;) {
      if (block_info.number == 0) {
        last_finalized_block = block_info.hash;
        break;
      }
      auto j_res = storage->getJustification(block_info.hash);
      if (j_res.has_value()) {
        last_finalized_block = block_info.hash;
        break;
      }
      if (j_res
          != outcome::failure(
              BlockStorageError::JUSTIFICATION_DOES_NOT_EXIST)) {
        return j_res.as_failure();
      }
      OUTCOME_TRY(header, storage->getBlockHeader(block_info.hash));
      block_info = {header.number - 1, header.parent_hash};
    }

    // create meta structures from the retrieved header
    OUTCOME_TRY(hash, header_repo->getHashById(last_finalized_block));
    OUTCOME_TRY(number, header_repo->getNumberById(last_finalized_block));

    std::optional<consensus::EpochNumber> curr_epoch_number;
    std::optional<consensus::EpochDigest> curr_epoch;
    std::optional<consensus::EpochDigest> next_epoch;
    auto hash_tmp = hash;

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
                   "EPOCH_DIGEST_IN_BLOCKTREE: CURR EPOCH #{}, Randomness: {}",
                   curr_epoch_number.value(),
                   curr_epoch.value().randomness.toHex());
        }
        if (not next_epoch.has_value()) {
          next_epoch.emplace(consensus::EpochDigest{
              .authorities = babe_configuration->genesis_authorities,
              .randomness = babe_configuration->randomness});
          SL_TRACE(log,
                   "EPOCH_DIGEST_IN_BLOCKTREE: NEXT EPOCH #{}+, Randomness: {}",
                   1,
                   next_epoch.value().randomness.toHex());
        }
        break;
      }

      OUTCOME_TRY(header_tmp, storage->getBlockHeader(hash_tmp));

      auto babe_digests_res = consensus::getBabeDigests(header_tmp);
      if (not babe_digests_res) {
        hash_tmp = header_tmp.parent_hash;
        continue;
      }

      auto slot_number = babe_digests_res.value().second.slot_number;
      auto epoch_number = babe_util->slotToEpoch(slot_number);

      SL_TRACE(log,
               "EPOCH_DIGEST_IN_BLOCKTREE: BLOCK, slot {}, epoch {}, block #{},"
               " hash {}",
               slot_number,
               epoch_number,
               header_tmp.number,
               hash_tmp.toHex());

      if (not curr_epoch_number.has_value()) {
        curr_epoch_number = epoch_number;
        SL_TRACE(log,
                 "EPOCH_DIGEST_IN_BLOCKTREE: CURRENT EPOCH #{}",
                 curr_epoch_number.value());
      }

      if (auto digest = consensus::getNextEpochDigest(header_tmp);
          digest.has_value()) {
        SL_TRACE(log,
                 "EPOCH_DIGEST_IN_BLOCKTREE: DIGEST, Randomness: {}",
                 digest.value().randomness.toHex());

        if (not next_epoch.has_value()) {
          next_epoch.emplace(digest.value());
          SL_TRACE(log,
                   "EPOCH_DIGEST_IN_BLOCKTREE: NEXT EPOCH #{}+, Randomness: {}",
                   epoch_number + 1,
                   next_epoch.value().randomness.toHex());
        }
        if (epoch_number != curr_epoch_number) {
          curr_epoch.emplace(digest.value());
          SL_TRACE(log,
                   "EPOCH_DIGEST_IN_BLOCKTREE: CURR EPOCH #{}, Randomness: {}",
                   curr_epoch_number.value(),
                   curr_epoch.value().randomness.toHex());
          break;
        }
      }

      hash_tmp = header_tmp.parent_hash;
    }

    SL_TRACE(log,
             "EPOCH_DIGEST_IN_BLOCKTREE: ROOT, block #{}, hash {}, "
             "Epoch {}, Current randomness {}, Next randomness {}",
             number,
             hash.toHex(),
             curr_epoch_number.value(),
             curr_epoch.value().randomness,
             next_epoch.value().randomness);

    auto tree = std::make_shared<TreeNode>(hash,
                                           number,
                                           std::move(curr_epoch.value()),
                                           curr_epoch_number.value(),
                                           std::move(next_epoch.value()),
                                           true);
    auto meta = std::make_shared<TreeMeta>(tree);

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
                          std::move(babe_configuration),
                          std::move(babe_util));
    return std::shared_ptr<BlockTreeImpl>(block_tree);
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
      std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
      std::shared_ptr<consensus::BabeUtil> babe_util)
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
        babe_configuration_(std::move(babe_configuration)),
        babe_util_(std::move(babe_util)) {
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
    BOOST_ASSERT(babe_configuration_ != nullptr);
    BOOST_ASSERT(babe_util_ != nullptr);

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

    OUTCOME_TRY(
        storage_->saveBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
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

    OUTCOME_TRY(
        storage_->saveBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
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

  outcome::result<void> BlockTreeImpl::addExistingBlock(
      const primitives::BlockHash &block_hash,
      const primitives::BlockHeader &block_header) {
    auto node = tree_->getRoot().findByHash(block_hash);
    // Check if tree doesn't have this block; if not, we skip that
    if (node != nullptr) {
      return BlockTreeError::BLOCK_EXISTS;
    }
    // Check if we know parent of this block; if not, we cannot insert it
    auto parent = tree_->getRoot().findByHash(block_header.parent_hash);
    if (parent == nullptr) {
      return BlockTreeError::NO_PARENT;
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

    OUTCOME_TRY(
        storage_->saveBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
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

    tree_->updateTreeRoot(node);

    OUTCOME_TRY(
        storage_->saveBlockTreeLeaves({tree_->getMetadata().leaves.begin(),
                                       tree_->getMetadata().leaves.end()}));

    // TODO(xDimon): After deploy of this change,
    //  setting of finalized block to storage should be removed
    OUTCOME_TRY(storage_->setLastFinalizedBlockHash(node->block_hash));

    OUTCOME_TRY(header, storage_->getBlockHeader(node->block_hash));

    chain_events_engine_->notify(
        primitives::events::ChainEventType::kFinalizedHeads, header);

    OUTCOME_TRY(new_runtime_version, runtime_core_->version(block_hash));
    if (not actual_runtime_version_.has_value()
        || actual_runtime_version_ != new_runtime_version) {
      actual_runtime_version_ = new_runtime_version;
      chain_events_engine_->notify(
          primitives::events::ChainEventType::kFinalizedRuntimeVersion,
          new_runtime_version);
    }
    OUTCOME_TRY(body, storage_->getBlockBody(node->block_hash));

    for (auto &ext : body) {
      if (auto key =
              extrinsic_event_key_repo_->get(hasher_->blake2b_256(ext.data))) {
        extrinsic_events_engine_->notify(
            key.value(),
            primitives::events::ExtrinsicLifecycleEvent::Finalized(key.value(),
                                                                   block_hash));
      }
    }

    log_->info("Finalized block {}",
               primitives::BlockInfo(node->depth, block_hash));
    metric_finalized_block_height_->set(node->depth);
    return outcome::success();
  }

  outcome::result<bool> BlockTreeImpl::hasBlockHeader(
      const primitives::BlockId &block) const {
    return storage_->hasBlockHeader(block);
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
      const primitives::BlockHash &block) const {
    return getChainByBlocks(
        tree_->getMetadata().last_finalized.lock()->block_hash, block);
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlock(
      const primitives::BlockHash &block,
      GetChainDirection direction,
      uint64_t maximum) const {
    auto block_number_res = header_repo_->getNumberByHash(block);
    if (!block_number_res) {
      log_->error("cannot retrieve block with hash {}: {}",
                  block.toHex(),
                  block_number_res.error().message());
      return BlockTreeError::NO_SUCH_BLOCK;
    }
    auto start_block_number = block_number_res.value();

    primitives::BlockNumber finish_block_number;  // NOLINT
    if (direction == GetChainDirection::DESCEND) {
      if (start_block_number < maximum) {
        // we want to finish at the root
        finish_block_number = 0;
      } else {
        // some non-root block
        finish_block_number = start_block_number - maximum + 1;
      }
    } else {
      finish_block_number = start_block_number + maximum - 1;
      auto current_depth = tree_->getMetadata().deepest_leaf.lock()->depth;
      if (current_depth < finish_block_number) {
        finish_block_number = current_depth;
      }
    }

    auto finish_block_hash_res =
        header_repo_->getHashByNumber(finish_block_number);
    if (not finish_block_hash_res.has_value()) {
      log_->error("cannot retrieve block with number {}: {}",
                  finish_block_number,
                  finish_block_hash_res.error().message());
      return BlockTreeError::NO_SUCH_BLOCK;
    }
    auto &finish_block_hash = finish_block_hash_res.value();

    if (direction == GetChainDirection::ASCEND) {
      return getChainByBlocks(block, finish_block_hash, maximum);
    }

    // the function returns the blocks in the chronological order, but we want a
    // reverted one in this case
    OUTCOME_TRY(chain, getChainByBlocks(finish_block_hash, block, maximum));
    std::reverse(chain.begin(), chain.end());
    return std::move(chain);
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
      auto &chain = chain_res.value();
      return std::move(chain);
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
        return BlockTreeError::NO_SOME_BLOCK_IN_CHAIN;
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
                 top_block.hash.toHex(),
                 bottom_block.hash.toHex());
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
     * or changing logic may make it obsolete
     * block numbers may be obtained somewhere else
     */
    primitives::BlockNumber ancestor_depth = 0u;
    primitives::BlockNumber descendant_depth = 0u;
    if (ancestor_node_ptr) {
      ancestor_depth = ancestor_node_ptr->depth;
    } else {
      auto header_res = header_repo_->getBlockHeader(ancestor);
      if (!header_res) {
        return false;
      }
      ancestor_depth = header_res.value().number;
    }
    if (descendant_node_ptr) {
      descendant_depth = descendant_node_ptr->depth;
    } else {
      auto header_res = header_repo_->getBlockHeader(descendant);
      if (!header_res) {
        return false;
      }
      descendant_depth = header_res.value().number;
    }
    if (descendant_depth < ancestor_depth) {
      SL_WARN(log_,
              "Ancestor block is lower. #{} {} in comparison to #{} {}",
              ancestor_depth,
              ancestor.toHex(),
              descendant_depth,
              descendant.toHex());
      return false;
    }

    // if both nodes are in our light tree, we can use this representation
    // only
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

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::longestPath() const {
    auto &&[_, block_hash] = deepestLeaf();
    return getChainByBlock(block_hash);
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
                  header_repo_->getNumberByHash(getLastFinalized().hash));
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
    return Error::BLOCK_NOT_FOUND;
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
    // if node is not in tree_ it must be finalized and thus have only one child
    OUTCOME_TRY(child_hash, header_repo_->getHashByNumber(header.number + 1));
    return outcome::success(std::vector<primitives::BlockHash>{child_hash});
  }

  primitives::BlockInfo BlockTreeImpl::getLastFinalized() const {
    const auto &last = tree_->getMetadata().last_finalized.lock();
    BOOST_ASSERT(last != nullptr);
    return primitives::BlockInfo{last->depth, last->block_hash};
  }

  outcome::result<consensus::EpochDigest> BlockTreeImpl::getEpochDescriptor(
      consensus::EpochNumber epoch_number,
      primitives::BlockHash block_hash) const {
    auto node = tree_->getRoot().findByHash(block_hash);
    if (node) {
      if (node->epoch_number != epoch_number) {
        return *node->next_epoch_digest;
      }
      return *node->epoch_digest;
    }
    return BlockTreeError::NO_SUCH_BLOCK;
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
         current_node
         && !(current_node->finalized && current_node->children.size() == 1);
         current_node = current_node->parent.lock()) {
      // DFS-on-deque
      to_remove.emplace_back();  // Waterbreak
      std::copy_if(current_node->children.begin(),
                   current_node->children.end(),
                   std::back_inserter(to_remove),
                   [&following_node](const auto &child) {
                     return child != following_node;
                   });
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
      auto block_body_res = storage_->getBlockBody(node->block_hash);
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
      OUTCOME_TRY(storage_->removeBlock(node->block_hash, node->depth));
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

}  // namespace kagome::blockchain
