/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/block_tree.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <unordered_set>

#include <libp2p/common/final_action.hpp>

#include "application/app_configuration.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree_error.hpp"
#include "consensus/timeline/types.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "network/extrinsic_observer.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/event_types.hpp"
#include "storage/trie/trie_storage.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"
#include "telemetry/service.hpp"
#include "utils/safe_object.hpp"
#include "utils/thread_pool.hpp"

namespace kagome::storage::trie_pruner {
  class TriePruner;
}

namespace kagome::blockchain {
  struct ReorgAndPrune;
  class TreeNode;
  class CachedTree;

  class BlockTreeImpl : public BlockTree,
                        public std::enable_shared_from_this<BlockTreeImpl> {
   public:
    /// Create an instance of block tree
    static outcome::result<std::shared_ptr<BlockTreeImpl>> create(
        const application::AppConfiguration &app_config,
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
        std::shared_ptr<::boost::asio::io_context> io_context);

    /// Recover block tree state at provided block
    static outcome::result<void> recover(
        primitives::BlockId target_block_id,
        std::shared_ptr<BlockStorage> storage,
        std::shared_ptr<BlockHeaderRepository> header_repo,
        std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
        std::shared_ptr<blockchain::BlockTree> block_tree);

    ~BlockTreeImpl() override = default;

    const primitives::BlockHash &getGenesisBlockHash() const override;

    outcome::result<std::optional<primitives::BlockHash>> getBlockHash(
        primitives::BlockNumber block_number) const override;

    outcome::result<bool> hasBlockHeader(
        const primitives::BlockHash &block_hash) const override;

    outcome::result<primitives::BlockHeader> getBlockHeader(
        const primitives::BlockHash &block_hash) const override;

    outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockHash &block_hash) const override;

    outcome::result<primitives::Justification> getBlockJustification(
        const primitives::BlockHash &block_hash) const override;

    outcome::result<void> addBlockHeader(
        const primitives::BlockHeader &header) override;

    outcome::result<void> addBlock(const primitives::Block &block) override;

    outcome::result<void> removeLeaf(
        const primitives::BlockHash &block_hash) override;

    outcome::result<void> addExistingBlock(
        const primitives::BlockHash &block_hash,
        const primitives::BlockHeader &block_header) override;

    outcome::result<void> markAsParachainDataBlock(
        const primitives::BlockHash &block_hash) override;

    outcome::result<void> markAsRevertedBlocks(
        const std::vector<primitives::BlockHash> &block_hashes) override;

    outcome::result<void> addBlockBody(
        const primitives::BlockHash &block_hash,
        const primitives::BlockBody &body) override;

    outcome::result<void> finalize(
        const primitives::BlockHash &block_hash,
        const primitives::Justification &justification) override;

    BlockHashVecRes getBestChainFromBlock(const primitives::BlockHash &block,
                                          uint64_t maximum) const override;

    BlockHashVecRes getDescendingChainToBlock(
        const primitives::BlockHash &block, uint64_t maximum) const override;

    BlockHashVecRes getChainByBlocks(
        const primitives::BlockHash &ancestor,
        const primitives::BlockHash &descendant) const override;

    bool hasDirectChain(const primitives::BlockHash &ancestor,
                        const primitives::BlockHash &descendant) const override;

    bool isFinalized(const primitives::BlockInfo &block) const override;

    primitives::BlockInfo bestBlock() const override;

    outcome::result<primitives::BlockInfo> getBestContaining(
        const primitives::BlockHash &target_hash) const override;

    std::vector<primitives::BlockHash> getLeaves() const override;

    BlockHashVecRes getChildren(
        const primitives::BlockHash &block) const override;

    primitives::BlockInfo getLastFinalized() const override;

    void warp(const primitives::BlockInfo &block_info) override;

    void notifyBestAndFinalized() override;

    void removeUnfinalized() override;

   private:
    struct BlocksPruning {
      BlocksPruning(std::optional<uint32_t> keep,
                    primitives::BlockNumber finalized);

      primitives::BlockNumber max(primitives::BlockNumber finalized) const;

      std::optional<uint32_t> keep_;
      primitives::BlockNumber next_;
    };

    struct BlockTreeData {
      std::shared_ptr<BlockHeaderRepository> header_repo_;
      std::shared_ptr<BlockStorage> storage_;
      std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner_;
      std::unique_ptr<CachedTree> tree_;
      std::shared_ptr<network::ExtrinsicObserver> extrinsic_observer_;
      std::shared_ptr<crypto::Hasher> hasher_;
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          extrinsic_event_key_repo_;
      std::shared_ptr<const class JustificationStoragePolicy>
          justification_storage_policy_;
      std::optional<primitives::BlockHash> genesis_block_hash_;
      BlocksPruning blocks_pruning_;
    };

    /**
     * Private constructor, so that instances are created only through the
     * factory method
     */
    BlockTreeImpl(
        const application::AppConfiguration &app_config,
        std::shared_ptr<BlockHeaderRepository> header_repo,
        std::shared_ptr<BlockStorage> storage,
        const primitives::BlockInfo &finalized,
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
        std::shared_ptr<::boost::asio::io_context> io_context);

    outcome::result<void> reorgAndPrune(BlockTreeData &p,
                                        const ReorgAndPrune &changes);

    outcome::result<primitives::BlockHeader> getBlockHeaderNoLock(
        const BlockTreeData &p, const primitives::BlockHash &block_hash) const;

    outcome::result<void> pruneTrie(const BlockTreeData &block_tree_data,
                                    primitives::BlockNumber new_finalized);

    primitives::BlockInfo getLastFinalizedNoLock(const BlockTreeData &p) const;
    primitives::BlockInfo bestBlockNoLock(const BlockTreeData &p) const;

    bool hasDirectChainNoLock(const BlockTreeData &p,
                              const primitives::BlockHash &ancestor,
                              const primitives::BlockHash &descendant) const;
    std::vector<primitives::BlockHash> getLeavesNoLock(
        const BlockTreeData &p) const;

    BlockTree::BlockHashVecRes getDescendingChainToBlockNoLock(
        const BlockTreeData &p,
        const primitives::BlockHash &to_block,
        uint64_t maximum) const;

    outcome::result<void> addExistingBlockNoLock(
        BlockTreeData &p,
        const primitives::BlockHash &block_hash,
        const primitives::BlockHeader &block_header);

    void notifyChainEventsEngine(primitives::events::ChainEventType event,
                                 const primitives::BlockHeader &header);

    class SafeBlockTreeData {
     public:
      SafeBlockTreeData(BlockTreeData data);

      template <typename F>
      decltype(auto) exclusiveAccess(F &&f) {
        // if this thread owns the mutex, it shall not be unlocked until this
        // function exits
        if (exclusive_owner_.load(std::memory_order_acquire)
            == std::this_thread::get_id()) {
          return f(block_tree_data_.unsafeGet());
        }
        return block_tree_data_.exclusiveAccess(
            [&f, this](BlockTreeData &data) {
              exclusive_owner_ = std::this_thread::get_id();
              ::libp2p::common::FinalAction reset(
                  [&] { exclusive_owner_ = std::nullopt; });
              return f(data);
            });
      }

      template <typename F>
      decltype(auto) sharedAccess(F &&f) const {
        // if this thread owns the mutex, it shall not be unlocked until this
        // function exits
        if (exclusive_owner_.load(std::memory_order_acquire)
            == std::this_thread::get_id()) {
          return f(block_tree_data_.unsafeGet());
        }
        return block_tree_data_.sharedAccess(std::forward<F>(f));
      }

     private:
      SafeObject<BlockTreeData> block_tree_data_;
      std::atomic<std::optional<std::thread::id>> exclusive_owner_;

    } block_tree_data_;

    primitives::events::ExtrinsicSubscriptionEnginePtr
        extrinsic_events_engine_ = {};
    primitives::events::ChainSubscriptionEnginePtr chain_events_engine_ = {};
    log::Logger log_ = log::createLogger("BlockTree", "block_tree");

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_best_block_height_;
    metrics::Gauge *metric_finalized_block_height_;
    metrics::Gauge *metric_known_chain_leaves_;
    ThreadHandler main_thread_;
    telemetry::Telemetry telemetry_ = telemetry::createTelemetryService();
  };
}  // namespace kagome::blockchain
