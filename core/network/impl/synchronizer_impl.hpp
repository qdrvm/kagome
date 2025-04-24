/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/synchronizer.hpp"

#include <atomic>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <unordered_set>

#include <libp2p/basic/scheduler.hpp>

#include "application/app_state_manager.hpp"
#include "application/sync_method.hpp"
#include "blockchain/block_storage.hpp"
#include "consensus/timeline/block_executor.hpp"
#include "consensus/timeline/block_header_appender.hpp"
#include "injector/lazy.hpp"
#include "metrics/metrics.hpp"
#include "network/impl/state_sync_request_flow.hpp"
#include "network/router.hpp"
#include "network/types/blocks_request.hpp"
#include "network/types/blocks_response.hpp"
#include "primitives/event_types.hpp"
#include "storage/spaced_storage.hpp"
#include "telemetry/service.hpp"
#include "utils/safe_object.hpp"

namespace kagome {
  class PoolHandlerReady;
}  // namespace kagome

namespace kagome::application {
  class AppConfiguration;
}

namespace kagome::blockchain {
  class BlockTree;
  class BlockStorage;
}  // namespace kagome::blockchain

namespace kagome::common {
  class MainThreadPool;
}

namespace kagome::consensus {
  class BlockHeaderAppender;
  class BlockExecutor;
  class Timeline;
  class SlotsUtil;
}  // namespace kagome::consensus

namespace kagome::consensus::grandpa {
  class Environment;
}  // namespace kagome::consensus::grandpa

namespace kagome::network {
  class Beefy;
  class PeerManager;
}  // namespace kagome::network

namespace kagome::storage::trie {
  class TrieSerializer;
  class TrieStorage;
}  // namespace kagome::storage::trie

namespace kagome::storage::trie_pruner {
  class TriePruner;
}

namespace kagome::network {
  class SynchronizerImpl
      : public Synchronizer,
        public std::enable_shared_from_this<SynchronizerImpl> {
   public:
    /// Minimum number of blocks to keep preloaded to ensure smooth chain
    /// processing. This allows simultaneous block application and preloading.
    /// Value 256 is chosen as twice the maximum number of blocks in a single
    /// BlocksResponse.
    static constexpr size_t kMinPreloadedBlockAmount = 256;

    /// Block amount enough for applying and preloading other ones
    /// simultaneously during fast syncing
    static constexpr size_t kMinPreloadedBlockAmountForFastSyncing =
        kMinPreloadedBlockAmount * 40;

    /// Indicating how far the block can be subscribed to.
    /// In general we don't needed wait very far blocks. This limit to avoid
    /// extra memory consumption.
    static constexpr size_t kMaxDistanceToBlockForSubscription =
        kMinPreloadedBlockAmount * 2;

    static constexpr std::chrono::milliseconds kRecentnessDuration =
        std::chrono::minutes{2};

    enum class Error {
      SHUTTING_DOWN = 1,
      EMPTY_RESPONSE,
      RESPONSE_WITHOUT_BLOCK_HEADER,
      RESPONSE_WITHOUT_BLOCK_BODY,
      DISCARDED_BLOCK,
      WRONG_ORDER,
      INVALID_HASH,
      ALREADY_IN_QUEUE,
      PEER_BUSY,
      ARRIVED_TOO_EARLY,
      DUPLICATE_REQUEST,
    };

    SynchronizerImpl(
        const application::AppConfiguration &app_config,
        application::AppStateManager &app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<clock::SystemClock> clock,
        LazySPtr<consensus::SlotsUtil> slots_util,
        std::shared_ptr<consensus::BlockHeaderAppender> block_appender,
        std::shared_ptr<consensus::BlockExecutor> block_executor,
        std::shared_ptr<storage::trie::TrieStorageBackend> trie_node_db,
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<storage::trie_pruner::TriePruner> trie_pruner,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<PeerManager> peer_manager,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        std::shared_ptr<crypto::Hasher> hasher,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        LazySPtr<consensus::Timeline> timeline,
        std::shared_ptr<Beefy> beefy,
        std::shared_ptr<consensus::grandpa::Environment> grandpa_environment,
        common::MainThreadPool &main_thread_pool,
        std::shared_ptr<blockchain::BlockStorage> block_storage);

    /** @see AppStateManager::takeControl */
    bool start();
    void stop();

    /// Enqueues loading (and applying) blocks from peer {@param peer_id}
    /// since best common block up to provided {@param block_info}.
    /// @note Is used for start/continue catching up.
    void addPeerKnownBlockInfo(const BlockInfo &block_info,
                               const PeerId &peer_id) override;

    /// Enqueues loading and applying block from peer {@param peer_id}
    /// based on the announced {@param header}.
    void onBlockAnnounce(const BlockHeader &header,
                         const PeerId &peer_id) override;

    bool fetchJustification(const primitives::BlockInfo &block,
                            CbResultVoid cb) override;

    bool fetchJustificationRange(primitives::BlockNumber min,
                                 FetchJustificationRangeCb cb) override;

    bool fetchHeadersBack(const primitives::BlockInfo &max,
                          primitives::BlockNumber min,
                          bool isFinalized,
                          CbResultVoid cb) override;

    /// Enqueues loading and applying state on block {@param block}
    /// from peer {@param peer_id}.
    /// If finished, {@param handler} be called
    void syncState(const BlockInfo &block, SyncStateCb handler) override;

    /// Fetches the fork headers from a specific peer.
    ///
    /// This function sends a request to the specified peer to retrieve
    /// information about a Grandpa fork based on the provided vote.
    ///
    /// @param peer_id The identifier of the peer from which the fork
    /// information is to be fetched.
    /// @param vote The block information representing the Grandpa vote
    ///             for which the fork data is being requested.
    void trySyncShortFork(const PeerId &peer_id,
                          const primitives::BlockInfo &block) override;

    /// Used for 'unsafe' sync. Fetches headers back from a peer until a block
    /// with justification for a Grandpa scheduled change is found.
    ///
    /// This function sends requests to the specified peer to retrieve block
    /// headers backward from the maximum block number provided, looking
    /// specifically for blocks containing authority set changes with
    /// justifications.
    ///
    /// @param peer_id The identifier of the peer from which to fetch the
    /// headers.
    /// @param max The block number to start fetching headers from.
    /// @param cb Callback that receives either a block number to use as the
    /// next maximum
    ///           or a pair containing a block header and its justification when
    ///           a valid authority set change with justification is found.
    void unsafe(PeerId peer_id, BlockNumber max, UnsafeCb cb) override;

    /// Loads blocks from the specified peer according to the request
    /// parameters.
    ///
    /// This method sends a request to fetch blocks from a peer and manages the
    /// fetching process, including tracking busy peers and handling responses.
    ///
    /// @param peer_id The identifier of the peer to request blocks from
    /// @param request The parameters specifying which blocks to request
    /// @param fetching Reference to a counter tracking active fetch operations
    /// @param reason Description of why blocks are being requested (for
    /// logging)
    void loadBlocks(const PeerId &peer_id,
                    BlocksRequest request,
                    size_t &fetching,
                    const char *reason);

   private:
    /// Updates the roots of the sync tree and manages block processing state.
    ///
    /// This method performs maintenance on attached and detached roots:
    /// - Removes roots that have been successfully applied to the block tree
    /// - Moves children of applied blocks to the attached roots
    /// - Removes blocks that can no longer be attached (invalid or orphaned)
    /// - Updates metrics and triggers additional fetch operations as needed
    void updateRoots();

    /// Pops next block from queue and tries to apply that
    void applyNextBlock();

    /// Purges internal cache of recent requests for specified {@param peer_id}
    /// and {@param fingerprint} after kRecentnessDuration timeout
    void scheduleRecentRequestRemoval(
        const libp2p::peer::PeerId &peer_id,
        const BlocksRequest::Fingerprint &fingerprint);

    void syncState();
    outcome::result<void> syncState(std::unique_lock<std::mutex> &lock,
                                    outcome::result<StateResponse> &&_res);

    void fetch(const libp2p::peer::PeerId &peer,
               BlocksRequest request,
               const char *reason,
               std::function<void(outcome::result<BlocksResponse>)> &&cb);

    std::optional<libp2p::peer::PeerId> chooseJustificationPeer(
        primitives::BlockNumber block, BlocksRequest::Fingerprint fingerprint);

    void afterStateSync();

    void randomWarp();

    void setHangTimer();
    void onHangTimer();

    /**
     * @brief Adds a block header to the synchronizer's known blocks
     *
     * This method processes a new block header received from a peer and adds it
     * to the synchronizer's internal state if valid. It updates the block
     * ancestry tree and manages attached/detached roots as necessary.
     *
     * @param peer_id The identifier of the peer that provided the block data
     * @param data Block data containing at least a header to be added
     * @return true if the header was successfully added, false otherwise
     */
    bool addHeader(const PeerId &peer_id, primitives::BlockData &&data);

    /**
     * @brief Executes various block fetching tasks based on the current
     * synchronization state
     *
     * This method implements the core block fetching logic of the synchronizer.
     * It:
     * 1. Fetches block bodies for attached blocks that are missing them
     * 2. Fetches ancestor blocks for detached blocks to try to connect them to
     * the chain
     * 3. Fetches new blocks beyond the current chain head when capacity allows
     *
     * The method respects download limits set by max_parallel_downloads_ and
     * tracks different types of fetches separately (body, gap, range).
     */
    void fetchTasks();

    /**
     * @brief Traverses the block ancestry tree starting from a root block
     *
     * This method performs a depth-first traversal of the block ancestry tree,
     * starting from the specified root block. For each block encountered, it
     * calls the provided callback function which can control the traversal
     * behavior.
     *
     * @param root The block information representing the starting point for
     * traversal
     * @param f Callback function that is called for each block in the ancestry
     * tree The callback should return a VisitAncestryResult value to control
     * the traversal:
     *          - CONTINUE: Visit children of the current block
     *          - STOP: Stop traversal completely
     *          - IGNORE_SUBTREE: Skip children of the current block but
     * continue traversal
     */
    void visitAncestry(const BlockInfo &root, const auto &f);
    /**
     * @brief Checks if more block fetching operations can be started
     *
     * This method determines whether the synchronizer has available capacity to
     * start additional block download operations. It compares the current
     * number of active fetches against the configured maximum parallel
     * downloads limit.
     *
     * @param fetching Reference to a counter tracking the current number of
     * active fetch operations
     * @return true if more blocks can be fetched, false if at or exceeding
     * capacity
     */
    bool canFetchMore(size_t &fetching) const;

    /**
     * @brief Validates that a blocks response matches the associated request
     * and performs several validation checks on the incoming block response
     *
     * @param request The original blocks request that was sent
     * @param response The blocks response to validate
     * @return true if the response is valid, false if validation fails
     */
    bool validate(const BlocksRequest &request,
                  const BlocksResponse &response) const;
    size_t highestAllowedBlock() const;
    /**
     * @brief Determines if a block header is valid for processing
     *
     * This method checks whether a block header meets the criteria for being
     * processed by the synchronizer. It verifies that:
     * - The block is not older than the last finalized block
     * - If it's the immediate successor to the finalized block, it has the
     * correct parent hash
     *
     * @param header The block header to validate
     * @return true if the block is allowed to be processed, false otherwise
     */
    bool isBlockAllowed(const BlockHeader &header) const;
    void removeBlock(const BlockInfo &block);
    void removeBlockRecursive(const BlockInfo &block);
    bool isSlotIncreasing(const BlockHeader &parent,
                          const BlockHeader &header) const;
    bool isFutureBlock(const BlockHeader &header) const;

    log::Logger log_;

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<clock::SystemClock> clock_;
    LazySPtr<consensus::SlotsUtil> slots_util_;
    std::shared_ptr<consensus::BlockHeaderAppender> block_appender_;
    std::shared_ptr<consensus::BlockExecutor> block_executor_;
    std::shared_ptr<storage::trie::TrieStorageBackend> trie_node_db_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<storage::trie_pruner::TriePruner> trie_pruner_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<PeerManager> peer_manager_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<crypto::Hasher> hasher_;
    LazySPtr<consensus::Timeline> timeline_;
    std::shared_ptr<Beefy> beefy_;
    std::shared_ptr<consensus::grandpa::Environment> grandpa_environment_;
    primitives::events::ChainSubscriptionEnginePtr chain_sub_engine_;
    std::shared_ptr<PoolHandlerReady> main_pool_handler_;
    std::shared_ptr<blockchain::BlockStorage> block_storage_;
    uint32_t max_parallel_downloads_;
    std::mt19937 random_gen_;

    application::SyncMethod sync_method_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_import_queue_length_;

    telemetry::Telemetry telemetry_ = telemetry::createTelemetryService();

    struct StateSync {
      libp2p::peer::PeerId peer;
      SyncStateCb cb;
    };

    mutable std::mutex state_sync_mutex_;
    std::optional<StateSyncRequestFlow> state_sync_flow_;
    std::optional<StateSync> state_sync_;

    bool node_is_shutting_down_ = false;

    using Ancestry = std::unordered_multimap<BlockInfo, BlockInfo>;
    struct KnownBlock {
      /// Data of block
      primitives::BlockData data;
      /// Peers who know this block
      std::unordered_set<PeerId> peers;
      Ancestry::iterator ancestry_it;
    };

    // Already known (enqueued) but is not applied yet
    std::unordered_map<primitives::BlockHash, KnownBlock> known_blocks_;
    // Set of blocks that are connected to the main chain
    std::set<BlockInfo> attached_roots_;
    // Set of blocks that are not connected to the main chain yet
    std::set<BlockInfo> detached_roots_;

    // Links parent->child
    Ancestry ancestry_;

    std::unordered_set<PeerId> busy_peers_;

    std::map<std::tuple<libp2p::peer::PeerId, BlocksRequest::Fingerprint>,
             const char *>
        recent_requests_;
    std::unordered_set<BlockInfo> executing_blocks_;

    libp2p::Cancel hang_timer_;

    size_t fetching_range_count_ = 0;
    size_t fetching_gap_count_ = 0;
    size_t fetching_body_count_ = 0;
  };

}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, SynchronizerImpl::Error)
