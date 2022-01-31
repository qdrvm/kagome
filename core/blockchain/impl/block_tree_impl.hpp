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

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/common.hpp"
#include "consensus/babe/babe_util.hpp"
#include "consensus/babe/common.hpp"
#include "consensus/babe/types/epoch_digest.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "network/extrinsic_observer.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/event_types.hpp"
#include "runtime/runtime_api/core.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"

namespace kagome::storage::changes_trie {
  class ChangesTracker;
}

namespace kagome::blockchain {

  class TreeNode;
  class CachedTree;

  class BlockTreeImpl : public BlockTree {
   public:
    enum class Error {
      // target block number is past the given maximum number
      TARGET_IS_PAST_MAX = 1,
      // block resides on a dead fork
      BLOCK_ON_DEAD_END,
      // block exists in chain but not found when following all
      // leaves backwards.
      BLOCK_NOT_FOUND,
    };

    /**
     * Create an instance of block tree
     * @param header_repo - block headers repository
     * @param storage - block storage for the tree to be put in
     * @param last_finalized_block - last finalized block, from which the tree
     * is going to grow
     * @param hasher - pointer to the hasher
     * @return ptr to the created instance or error
     */
    static outcome::result<std::shared_ptr<BlockTreeImpl>> create(
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
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
        std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
        std::shared_ptr<consensus::BabeUtil> babe_util);

    ~BlockTreeImpl() override = default;

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

    BlockHashVecRes getChainByBlock(
        const primitives::BlockHash &block) const override;

    BlockHashVecRes getChainByBlocks(const primitives::BlockHash &top_block,
                                     const primitives::BlockHash &bottom_block,
                                     uint32_t max_count) const override;

    BlockHashVecRes getChainByBlock(const primitives::BlockHash &block,
                                    GetChainDirection ascending,
                                    uint64_t maximum) const override;

    BlockHashVecRes getChainByBlocks(
        const primitives::BlockHash &top_block,
        const primitives::BlockHash &bottom_block) const override;

    std::optional<primitives::Version> runtimeVersion() const override {
      return actual_runtime_version_;
    }

    bool hasDirectChain(const primitives::BlockHash &ancestor,
                        const primitives::BlockHash &descendant) const override;

    BlockHashVecRes longestPath() const override;

    primitives::BlockInfo deepestLeaf() const override;

    outcome::result<primitives::BlockInfo> getBestContaining(
        const primitives::BlockHash &target_hash,
        const std::optional<primitives::BlockNumber> &max_number)
        const override;

    std::vector<primitives::BlockHash> getLeaves() const override;

    BlockHashVecRes getChildren(
        const primitives::BlockHash &block) const override;

    primitives::BlockInfo getLastFinalized() const override;

    outcome::result<consensus::EpochDigest> getEpochDescriptor(
        consensus::EpochNumber epoch_number,
        primitives::BlockHash block_hash) const override;

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
        std::shared_ptr<runtime::Core> runtime_core,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
        std::shared_ptr<primitives::BabeConfiguration> babe_configuration,
        std::shared_ptr<consensus::BabeUtil> babe_util);

    /**
     * Walks the chain backwards starting from \param start until the current
     * block number is less or equal than \param limit
     */
    outcome::result<primitives::BlockHash> walkBackUntilLess(
        const primitives::BlockHash &start,
        const primitives::BlockNumber &limit) const;

    std::optional<std::vector<primitives::BlockHash>>
    tryGetChainByBlocksFromCache(const primitives::BlockInfo &top_block,
                                 const primitives::BlockInfo &bottom_block,
                                 std::optional<uint32_t> max_count) const;

    BlockHashVecRes getChainByBlocks(const primitives::BlockHash &top_block,
                                     const primitives::BlockHash &bottom_block,
                                     std::optional<uint32_t> max_count) const;

    /**
     * @returns the tree leaves sorted by their depth
     */
    std::vector<primitives::BlockHash> getLeavesSorted() const;

    outcome::result<void> prune(
        const std::shared_ptr<TreeNode> &lastFinalizedNode);

    std::shared_ptr<BlockHeaderRepository> header_repo_;
    std::shared_ptr<BlockStorage> storage_;

    std::unique_ptr<CachedTree> tree_;

    std::shared_ptr<network::ExtrinsicObserver> extrinsic_observer_;

    std::shared_ptr<crypto::Hasher> hasher_;
    primitives::events::ChainSubscriptionEnginePtr chain_events_engine_;
    primitives::events::ExtrinsicSubscriptionEnginePtr extrinsic_events_engine_;
    std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
        extrinsic_event_key_repo_;
    std::shared_ptr<runtime::Core> runtime_core_;
    std::shared_ptr<storage::changes_trie::ChangesTracker>
        trie_changes_tracker_;
    std::shared_ptr<primitives::BabeConfiguration> babe_configuration_;
    std::shared_ptr<const consensus::BabeUtil> babe_util_;
    std::optional<primitives::Version> actual_runtime_version_;
    log::Logger log_ = log::createLogger("BlockTree", "blockchain");

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_best_block_height_;
    metrics::Gauge *metric_finalized_block_height_;
    metrics::Gauge *metric_known_chain_leaves_;
  };
}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, BlockTreeImpl::Error);

#endif  // KAGOME_BLOCK_TREE_IMPL_HPP
