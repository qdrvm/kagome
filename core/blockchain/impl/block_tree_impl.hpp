/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_TREE_IMPL_HPP
#define KAGOME_BLOCK_TREE_IMPL_HPP

#include "blockchain/block_tree.hpp"

#include <functional>
#include <memory>
#include <queue>
#include <unordered_set>

#include <optional>

#include "application/app_configuration.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/common.hpp"
#include "consensus/babe/common.hpp"
#include "consensus/babe/types/epoch_digest.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "network/extrinsic_observer.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/event_types.hpp"
#include "storage/trie/trie_storage.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"
#include "telemetry/service.hpp"

namespace kagome::storage::trie_pruner {
  class TriePruner;
}

namespace kagome::storage::changes_trie {
  class ChangesTracker;
}

namespace kagome::blockchain {

  class TreeNode;
  class CachedTree;

  class BlockTreeImpl : public BlockTree,
                        public std::enable_shared_from_this<BlockTreeImpl> {
   public:
    /// Create an instance of block tree
    static outcome::result<std::shared_ptr<BlockTreeImpl>> create(
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
            justification_storage_policy,
        std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner);

    /// Recover block tree state at provided block
    static outcome::result<void> recover(
        primitives::BlockId target_block,
        std::shared_ptr<BlockStorage> storage,
        std::shared_ptr<BlockHeaderRepository> header_repo,
        std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
        std::shared_ptr<blockchain::BlockTree> block_tree);

    ~BlockTreeImpl() override = default;

    const primitives::BlockHash &getGenesisBlockHash() const override;

    outcome::result<bool> hasBlockHeader(
        const primitives::BlockId &block) const override;

    outcome::result<primitives::BlockHeader> getBlockHeader(
        const primitives::BlockId &block) const override;

    outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockId &block) const override;

    outcome::result<primitives::Justification> getBlockJustification(
        const primitives::BlockId &block) const override;

    outcome::result<void> addBlockHeader(
        const primitives::BlockHeader &header) override;

    outcome::result<void> addBlock(const primitives::Block &block) override;

    outcome::result<void> removeLeaf(
        const primitives::BlockHash &block_hash) override;

    outcome::result<void> addExistingBlock(
        const primitives::BlockHash &block_hash,
        const primitives::BlockHeader &block_header) override;

    outcome::result<void> addBlockBody(
        primitives::BlockNumber block_number,
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

    primitives::BlockInfo bestLeaf() const override;

    outcome::result<primitives::BlockInfo> getBestContaining(
        const primitives::BlockHash &target_hash,
        const std::optional<primitives::BlockNumber> &max_number)
        const override;

    std::vector<primitives::BlockHash> getLeaves() const override;

    BlockHashVecRes getChildren(
        const primitives::BlockHash &block) const override;

    primitives::BlockInfo getLastFinalized() const override;

   private:
    /**
     * Private constructor, so that instances are created only through the
     * factory method
     */
    BlockTreeImpl(
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
        std::shared_ptr<const class JustificationStoragePolicy>
            justification_storage_policy,
        std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner);

    /**
     * Walks the chain backwards starting from \param start until the current
     * block number is less or equal than \param limit
     */
    outcome::result<primitives::BlockHash> walkBackUntilLess(
        const primitives::BlockHash &start,
        const primitives::BlockNumber &limit) const;

    /**
     * @returns the tree leaves sorted by their depth
     */
    std::vector<primitives::BlockHash> getLeavesSorted() const;

    outcome::result<void> prune(
        const std::shared_ptr<TreeNode> &lastFinalizedNode);

    outcome::result<void> reorganize();

    std::shared_ptr<BlockHeaderRepository> header_repo_;
    std::shared_ptr<BlockStorage> storage_;
    std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner_;

    std::unique_ptr<CachedTree> tree_;

    std::shared_ptr<network::ExtrinsicObserver> extrinsic_observer_;

    std::shared_ptr<crypto::Hasher> hasher_;
    primitives::events::ChainSubscriptionEnginePtr chain_events_engine_;
    primitives::events::ExtrinsicSubscriptionEnginePtr extrinsic_events_engine_;
    std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
        extrinsic_event_key_repo_;
    std::shared_ptr<storage::changes_trie::ChangesTracker>
        trie_changes_tracker_;
    std::shared_ptr<const class JustificationStoragePolicy>
        justification_storage_policy_;

    std::optional<primitives::BlockHash> genesis_block_hash_;

    log::Logger log_ = log::createLogger("BlockTree", "block_tree");

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_best_block_height_;
    metrics::Gauge *metric_finalized_block_height_;
    metrics::Gauge *metric_known_chain_leaves_;
    telemetry::Telemetry telemetry_ = telemetry::createTelemetryService();
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_TREE_IMPL_HPP
