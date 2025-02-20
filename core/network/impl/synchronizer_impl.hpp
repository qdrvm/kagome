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
    /// Block amount enough for applying and preloading other ones
    /// simultaneously.
    /// 256 is doubled max amount block in BlocksResponse.
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
        std::chrono::seconds(60);

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
    /// {@param handler} will be called when this process is finished or failed
    /// @returns true if sync is ran (peer is not busy)
    /// @note Is used for start/continue catching up.
    bool syncByBlockInfo(const primitives::BlockInfo &block_info,
                         const libp2p::peer::PeerId &peer_id,
                         SyncResultHandler &&handler,
                         bool subscribe_to_block) override;

    /// Enqueues loading and applying block {@param block_info} from peer
    /// {@param peer_id}.
    /// @returns true if sync is ran (peer is not busy)
    /// If provided block is the best after applying, {@param handler} be called
    bool syncByBlockHeader(const primitives::BlockHeader &header,
                           const libp2p::peer::PeerId &peer_id,
                           SyncResultHandler &&handler) override;

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
    void syncState(const libp2p::peer::PeerId &peer_id,
                   const primitives::BlockInfo &block,
                   SyncResultHandler &&handler) override;

    /// Finds best common block with peer {@param peer_id} in provided interval.
    /// It is using tail-recursive algorithm, till {@param hint} is
    /// the needed block
    /// @param lower is number of definitely known common block (i.e. last
    /// finalized).
    /// @param upper is number of definitely unknown block.
    /// @param hint is examining block for current call
    /// @param handler callback what is called at the end of process
    void findCommonBlock(const libp2p::peer::PeerId &peer_id,
                         primitives::BlockNumber lower,
                         primitives::BlockNumber upper,
                         primitives::BlockNumber hint,
                         SyncResultHandler &&handler,
                         std::map<primitives::BlockNumber,
                                  primitives::BlockHash> &&observed = {});

    /// Loads blocks from peer {@param peer_id} since block {@param from} till
    /// its best. Calls {@param handler} when process is finished or failed
    void loadBlocks(const libp2p::peer::PeerId &peer_id,
                    primitives::BlockInfo from,
                    SyncResultHandler &&handler);

   private:
    void postApplyBlock();
    void processBlockAdditionResult(outcome::result<void> block_addition_result,
                                    const primitives::BlockHash &hash,
                                    SyncResultHandler &&handler);
    /// Subscribes handler for block with provided {@param block_info}
    /// {@param handler} will be called When block is received or discarded
    /// @returns true if subscription is successful
    bool subscribeToBlock(const primitives::BlockInfo &block_info,
                          SyncResultHandler &&handler);

    /// Notifies subscribers about arrived block
    void notifySubscribers(const primitives::BlockInfo &block_info,
                           outcome::result<void> res);

    /// Tries to request another portion of block
    void askNextPortionOfBlocks();

    void post_block_addition(outcome::result<void> block_addition_result,
                             Synchronizer::SyncResultHandler &&handler,
                             const primitives::BlockHash &hash);

    /// Pops next block from queue and tries to apply that
    void applyNextBlock();

    /// Removes block {@param block} and all all dependent on it from the queue
    /// @returns number of affected blocks
    size_t discardBlock(const primitives::BlockHash &block);

    /// Removes blocks what will never be applied because they are contained in
    /// side-branch for provided finalized block {@param finalized_block}
    void prune(const primitives::BlockInfo &finalized_block);

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

    log::Logger log_;

    std::shared_ptr<blockchain::BlockTree> block_tree_;
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

    application::SyncMethod sync_method_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_import_queue_length_;

    telemetry::Telemetry telemetry_ = telemetry::createTelemetryService();

    struct StateSync {
      libp2p::peer::PeerId peer;
      SyncResultHandler cb;
    };

    mutable std::mutex state_sync_mutex_;
    std::optional<StateSyncRequestFlow> state_sync_flow_;
    std::optional<StateSync> state_sync_;

    bool node_is_shutting_down_ = false;

    struct KnownBlock {
      /// Data of block
      primitives::BlockData data;
      /// Peers who know this block
      std::set<libp2p::peer::PeerId> peers;
    };

    // Already known (enqueued) but is not applied yet
    std::unordered_map<primitives::BlockHash, KnownBlock> known_blocks_;

    // Blocks grouped by number
    std::set<primitives::BlockInfo> generations_;

    // Links parent->child
    std::unordered_multimap<primitives::BlockHash, primitives::BlockHash>
        ancestry_;

    // BlockNumber of blocks (aka height) that is potentially best now
    primitives::BlockNumber watched_blocks_number_{};

    // Handlers what will be called when block is apply
    std::unordered_multimap<primitives::BlockHash, SyncResultHandler>
        watched_blocks_;

    std::multimap<primitives::BlockInfo, SyncResultHandler> subscriptions_;

    std::atomic_bool asking_blocks_portion_in_progress_ = false;
    std::set<libp2p::peer::PeerId> busy_peers_;
    std::unordered_set<primitives::BlockInfo> load_blocks_;
    std::pair<primitives::BlockNumber, std::chrono::milliseconds>
        load_blocks_max_{};

    std::map<std::tuple<libp2p::peer::PeerId, BlocksRequest::Fingerprint>,
             const char *>
        recent_requests_;
  };

}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, SynchronizerImpl::Error)
