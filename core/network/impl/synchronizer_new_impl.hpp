/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SYNCHRONIZERNEWIMPL
#define KAGOME_NETWORK_SYNCHRONIZERNEWIMPL

#include "network/synchronizer.hpp"

#include <atomic>
#include <queue>

#include <libp2p/basic/scheduler.hpp>

#include "application/app_state_manager.hpp"
#include "consensus/babe/block_appender.hpp"
#include "consensus/babe/block_executor.hpp"
#include "metrics/metrics.hpp"
#include "network/router.hpp"

namespace kagome::application {
  class AppConfiguration;
}

namespace kagome::storage::trie {
  class PersistentTrieBatch;
  class TrieSerializer;
  class TrieStorage;
}  // namespace kagome::storage::trie

namespace kagome::network {

  class SynchronizerNewImpl
      : public Synchronizer,
        public std::enable_shared_from_this<SynchronizerNewImpl> {
   public:
    /// Block amount enough for applying and preloading other ones
    /// simultaneously.
    /// 256 is doubled max amount block in BlocksResponse.
    static constexpr size_t kMinPreloadedBlockAmount = 256;

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
      DUPLICATE_REQUEST
    };

    SynchronizerNewImpl(
        const application::AppConfiguration &app_config,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<consensus::BlockAppender> block_appender,
        std::shared_ptr<consensus::BlockExecutor> block_executor,
        std::shared_ptr<storage::trie::TrieSerializer> serializer,
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        std::shared_ptr<crypto::Hasher> hasher);

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

    void syncState(const libp2p::peer::PeerId &peer_id,
                   const primitives::BlockInfo &block,
                   const common::Buffer &key,
                   SyncResultHandler &&handler) override;

    /// Loads blocks from peer {@param peer_id} since block {@param from} till
    /// its best. Calls {@param handler} when process is finished or failed
    void loadBlocks(const libp2p::peer::PeerId &peer_id,
                    primitives::BlockInfo from,
                    SyncResultHandler &&handler);

    void endSync() override {
      syncing_.store(false);
    }

   private:
    /// Subscribes handler for block with provided {@param block_info}
    /// {@param handler} will be called When block is received or discarded
    /// @returns true if subscription is successful
    bool subscribeToBlock(const primitives::BlockInfo &block_info,
                          SyncResultHandler &&handler);

    /// Notifies subscribers about arrived block
    void notifySubscribers(const primitives::BlockInfo &block_info,
                           outcome::result<void> res);

    /// Pops next block from queue and tries to apply that
    void applyNextBlock();

    /// Removes block {@param block} and all all dependent on it from the queue
    /// @returns number of affected blocks
    size_t discardBlock(const primitives::BlockHash &block);

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<consensus::BlockAppender> block_appender_;
    std::shared_ptr<consensus::BlockExecutor> block_executor_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<crypto::Hasher> hasher_;
    application::AppConfiguration::SyncMethod sync_method_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_import_queue_length_;

    log::Logger log_ = log::createLogger("Synchronizer", "synchronizer");

    std::atomic_bool syncing_ = false;
    std::atomic_bool state_syncing_ = false;
    bool node_is_shutting_down_ = false;

    struct KnownBlock {
      /// Data of block
      primitives::BlockData data;
      /// Peers who know this block
      std::set<libp2p::peer::PeerId> peers;
    };

    // Already known (enqueued) but is not applied yet
    std::unordered_map<primitives::BlockHash, KnownBlock> known_blocks_;

    std::optional<primitives::BlockInfo> sync_block_;

    // Blocks grouped by number
    std::multimap<primitives::BlockNumber, primitives::BlockHash> generations_;

    // Links parent->child
    std::unordered_multimap<primitives::BlockHash, primitives::BlockHash>
        ancestry_;

    std::multimap<primitives::BlockInfo, SyncResultHandler> subscriptions_;

    std::atomic_bool applying_in_progress_ = false;

    std::shared_ptr<storage::trie::PersistentTrieBatch> batch_;
    size_t entries_{0};
  };

}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, SynchronizerNewImpl::Error)

#endif  //  KAGOME_NETWORK_SYNCHRONIZERNEWIMPL
