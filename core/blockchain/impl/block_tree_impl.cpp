/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_tree_impl.hpp"

#include <algorithm>

#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/common.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "crypto/blake2/blake2b.h"
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

constexpr const char *kBlockHeightGaugeName = "kagome_block_height";

namespace kagome::blockchain {
  using Buffer = common::Buffer;
  using Prefix = prefix::Prefix;
  using DatabaseError = kagome::storage::DatabaseError;

  BlockTreeImpl::TreeNode::TreeNode(primitives::BlockHash hash,
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

  BlockTreeImpl::TreeNode::TreeNode(
      primitives::BlockHash hash,
      primitives::BlockNumber depth,
      const std::shared_ptr<TreeNode> &parent,
      consensus::EpochNumber epoch_number,
      boost::optional<consensus::EpochDigest> next_epoch_digest_opt,
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

  boost::optional<std::vector<std::shared_ptr<BlockTreeImpl::TreeNode>>>
  BlockTreeImpl::TreeNode::getPathTo(const primitives::BlockHash &hash) {
    std::vector<std::shared_ptr<TreeNode>> stack{shared_from_this()};
    std::unordered_map<primitives::BlockNumber, size_t> forks;

    bool came_back = false;
    while (not stack.empty()) {
      auto &current = stack.back();

      // Found
      if (current->block_hash == hash) {
        return stack;
      }

      // Straight
      if (current->children.size() == 1) {
        if (not came_back) {
          // Go to children
          stack.push_back(current->children.front());
          continue;
        }
        // else - continue going back
      }

      // Fork
      if (current->children.size() > 1) {
        auto &children_index = forks[current->depth];
        if (children_index < current->children.size()) {
          // Go to next child
          stack.push_back(current->children[children_index++]);
          came_back = false;
          continue;
        }
        // else (no more children) - going back
      }

      // Going back
      forks.erase(current->depth);
      stack.pop_back();
      came_back = true;
    }
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
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          extrinsic_event_key_repo,
      std::shared_ptr<runtime::Core> runtime_core,
      std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
      std::shared_ptr<consensus::BabeUtil> babe_util) {
    // create meta structures from the retrieved header
    OUTCOME_TRY(hash, header_repo->getHashById(last_finalized_block));
    OUTCOME_TRY(number, header_repo->getNumberById(last_finalized_block));

    log::Logger log = log::createLogger("BlockTree", "blockchain");

    boost::optional<consensus::EpochNumber> curr_epoch_number;
    boost::optional<consensus::EpochDigest> curr_epoch;
    boost::optional<consensus::EpochDigest> next_epoch;
    auto hash_tmp = hash;

    // We are going block by block to genesis direction and observes them for
    // find epoch digest. First found digest if it in the block assigned to the
    // current epoch will be saved as digest planned for next epoch. First found
    // digest if it in the block assigned to the early epoch will be saved as
    // digest for current epoch (and planned for next epoch, if it not defined
    // yet).

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
             "EPOCH_DIGEST_IN_BLOCKTREE: ROOT, block #{}, hash {}\n"
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
                                           std::move(next_epoch.value()));
    auto meta = std::make_shared<TreeMeta>(*tree);

    auto block_tree = new BlockTreeImpl(std::move(header_repo),
                                        std::move(storage),
                                        std::move(tree),
                                        std::move(meta),
                                        std::move(extrinsic_observer),
                                        std::move(hasher),
                                        std::move(chain_events_engine),
                                        std::move(extrinsic_events_engine),
                                        std::move(extrinsic_event_key_repo),
                                        std::move(runtime_core),
                                        std::move(babe_configuration),
                                        std::move(babe_util));
    return std::shared_ptr<BlockTreeImpl>(block_tree);
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
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          extrinsic_event_key_repo,
      std::shared_ptr<runtime::Core> runtime_core,
      std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
      std::shared_ptr<consensus::BabeUtil> babe_util)
      : header_repo_{std::move(header_repo)},
        storage_{std::move(storage)},
        tree_{std::move(tree)},
        tree_meta_{std::move(meta)},
        extrinsic_observer_{std::move(extrinsic_observer)},
        hasher_{std::move(hasher)},
        chain_events_engine_(std::move(chain_events_engine)),
        extrinsic_events_engine_(std::move(extrinsic_events_engine)),
        extrinsic_event_key_repo_{std::move(extrinsic_event_key_repo)},
        runtime_core_(std::move(runtime_core)),
        babe_configuration_(std::move(babe_configuration)),
        babe_util_(std::move(babe_util)) {
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(tree_ != nullptr);
    BOOST_ASSERT(tree_meta_ != nullptr);
    BOOST_ASSERT(extrinsic_observer_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(chain_events_engine_ != nullptr);
    BOOST_ASSERT(extrinsic_events_engine_ != nullptr);
    BOOST_ASSERT(extrinsic_event_key_repo_ != nullptr);
    BOOST_ASSERT(runtime_core_ != nullptr);
    BOOST_ASSERT(babe_configuration_ != nullptr);
    BOOST_ASSERT(babe_util_ != nullptr);
    // initialize metrics
    registry_->registerGaugeFamily(kBlockHeightGaugeName,
                                     "Block height info of the chain");
    block_height_best_ = registry_->registerGaugeMetric(kBlockHeightGaugeName,
                                                          {{"status", "best"}});
    block_height_best_->set(tree_meta_->deepest_leaf.get().depth);
    block_height_finalized_ = registry_->registerGaugeMetric(
        kBlockHeightGaugeName, {{"status", "finalized"}});
    block_height_finalized_->set(tree_meta_->last_finalized.get().depth);
  }

  outcome::result<void> BlockTreeImpl::addBlockHeader(
      const primitives::BlockHeader &header) {
    auto parent = tree_->getByHash(header.parent_hash);
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

    boost::optional<consensus::EpochDigest> next_epoch;
    if (auto digest = consensus::getNextEpochDigest(header);
        digest.has_value()) {
      next_epoch.emplace(std::move(digest.value()));
    }

    // update local meta with the new block
    auto new_node = std::make_shared<TreeNode>(
        block_hash, header.number, parent, epoch_number, std::move(next_epoch));
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

    consensus::EpochNumber epoch_number = 0;
    auto babe_digests_res = consensus::getBabeDigests(block.header);
    if (babe_digests_res.has_value()) {
      auto babe_slot = babe_digests_res.value().second.slot_number;
      epoch_number = babe_util_->slotToEpoch(babe_slot);
    }

    boost::optional<consensus::EpochDigest> next_epoch;
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

    updateMeta(new_node);
    chain_events_engine_->notify(primitives::events::ChainEventType::kNewHeads,
                                 block.header);
    for (size_t idx = 0; idx < block.body.size(); idx++) {
      if (auto key = extrinsic_event_key_repo_->getEventKey(block.header.number,
                                                            idx)) {
        extrinsic_events_engine_->notify(
            key.value(),
            primitives::events::ExtrinsicLifecycleEvent::InBlock(
                key.value(), std::move(block_hash)));
      }
    }

    block_height_best_->set(tree_meta_->deepest_leaf.get().depth);

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

    consensus::EpochNumber epoch_number = 0;
    auto babe_digests_res = consensus::getBabeDigests(block_header);
    if (babe_digests_res.has_value()) {
      auto babe_slot = babe_digests_res.value().second.slot_number;
      epoch_number = babe_util_->slotToEpoch(babe_slot);
    }

    boost::optional<consensus::EpochDigest> next_epoch;
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

    OUTCOME_TRY(new_runtime_version, runtime_core_->version());
    if (not actual_runtime_version_.has_value()
        || actual_runtime_version_ != new_runtime_version) {
      actual_runtime_version_ = new_runtime_version;
      chain_events_engine_->notify(
          primitives::events::ChainEventType::kRuntimeVersion,
          new_runtime_version);
    }
    OUTCOME_TRY(body, storage_->getBlockBody(node->block_hash));

    for (size_t idx = 0; idx < body.size(); idx++) {
      if (auto key =
              extrinsic_event_key_repo_->getEventKey(header.number, idx)) {
        extrinsic_events_engine_->notify(
            key.value(),
            primitives::events::ExtrinsicLifecycleEvent::Finalized(
                key.value(), std::move(block_hash)));
      }
    }

    log_->info("Finalized block. Number: {}, Hash: {}",
               node->depth,
               block_hash.toHex());
    block_height_finalized_->set(node->depth);
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
      const primitives::BlockHash &block) const {
    return getChainByBlocks(tree_meta_->last_finalized.get().block_hash, block);
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

    if (direction == GetChainDirection::ASCEND) {
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
      const uint32_t max_count) const {
    return getChainByBlocks(
        top_block, bottom_block, boost::make_optional(max_count));
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlocks(
      const primitives::BlockHash &top_block,
      const primitives::BlockHash &bottom_block,
      boost::optional<uint32_t> max_count) const {
    if (auto from_cache =
            tryGetChainByBlocksFromCache(top_block, bottom_block, max_count)) {
      return std::move(from_cache.value());
    }

    OUTCOME_TRY(from, header_repo_->getNumberByHash(top_block));
    OUTCOME_TRY(to, header_repo_->getNumberByHash(bottom_block));

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
    while (current_hash != top_block && result.size() < response_length) {
      auto header_res = header_repo_->getBlockHeader(current_hash);
      if (!header_res) {
        log_->warn(
            "impossible to get chain by blocks: "
            "intermediate block hash={} was not added to block tree before",
            current_hash.toHex());
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

  boost::optional<std::vector<primitives::BlockHash>>
  BlockTreeImpl::tryGetChainByBlocksFromCache(
      const primitives::BlockHash &top_block,
      const primitives::BlockHash &bottom_block,
      boost::optional<uint32_t> max_count) const {
    if (auto from = tree_->getByHash(top_block)) {
      if (auto way_opt = from->getPathTo(bottom_block)) {
        auto &way = way_opt.value();
        const auto in_tree_branch_len = way.back()->depth - from->depth + 1;
        const auto response_length =
            max_count ? std::min(in_tree_branch_len, max_count.value())
                      : in_tree_branch_len;
        SL_TRACE(log_,
                 "Create {} length chain from number {} to {} from cache.",
                 response_length,
                 from->depth,
                 way.back()->depth);

        way.resize(response_length);

        std::vector<primitives::BlockHash> result;
        result.reserve(response_length);
        for (auto &s : way) {
          result.emplace_back(s->block_hash);
        }

        return result;
      }
    }
    return boost::none;
  }

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::getChainByBlocks(
      const primitives::BlockHash &top_block,
      const primitives::BlockHash &bottom_block) const {
    return getChainByBlocks(top_block, bottom_block, boost::none);
  }

  bool BlockTreeImpl::hasDirectChain(
      const primitives::BlockHash &ancestor,
      const primitives::BlockHash &descendant) const {
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

  BlockTreeImpl::BlockHashVecRes BlockTreeImpl::longestPath() const {
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

  outcome::result<consensus::EpochDigest> BlockTreeImpl::getEpochDescriptor(
      consensus::EpochNumber epoch_number,
      primitives::BlockHash block_hash) const {
    auto node = tree_->getByHash(block_hash);
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
      auto leaf_node = tree_->getByHash(leaf);
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
        for (size_t idx = 0; idx < block_body_res.value().size(); idx++) {
          if (auto key = extrinsic_event_key_repo_->getEventKey(number, idx)) {
            extrinsic_events_engine_->notify(
                key.value(),
                primitives::events::ExtrinsicLifecycleEvent::Retracted(
                    key.value(), std::move(hash)));
          }
          extrinsics.emplace_back(std::move(block_body_res.value()[idx]));
        }
      }

      OUTCOME_TRY(storage_->removeBlock(hash, number));
    }

    // trying to return back extrinsics to transaction pool
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
