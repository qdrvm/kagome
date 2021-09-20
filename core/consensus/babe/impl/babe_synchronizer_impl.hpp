/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_SYNCHRONIZERIMPL2
#define KAGOME_CONSENSUS_BABE_SYNCHRONIZERIMPL2

#include "consensus/babe/babe_synchronizer.hpp"

#include <queue>

#include <libp2p/basic/scheduler.hpp>

#include "application/app_state_manager.hpp"
#include "consensus/babe/block_executor.hpp"
#include "network/router.hpp"

namespace kagome::consensus {

  class BabeSynchronizerImpl
      : public BabeSynchronizer,
        public std::enable_shared_from_this<BabeSynchronizerImpl> {
   public:
    static const size_t kMinPreloadedBlockNumber = 250;

    enum class Error {
      SHUTTING_DOWN = 1,
      EMPTY_RESPONSE,
      RESPONSE_WITHOUT_BLOCK_HEADER,
      RESPONSE_WITHOUT_BLOCK_BODY,
      DISCARDED_BLOCK,
      WRONG_ORDER,
      INVALID_HASH,
      ALREADY_IN_QUEUE,
      PEER_BUSY
    };

    BabeSynchronizerImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<BlockExecutor> block_executor,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        std::shared_ptr<crypto::Hasher> hasher);

    /// Enqueues loading (and applying) blocks from peer {@param peer_id}
    /// since best common block up to provided {@param block_info}.
    /// {@param handler} will be called when this process is finished or failed
    void enqueue(const primitives::BlockInfo &block_info,
                 const libp2p::peer::PeerId &peer_id,
                 SyncResultHandler &&handler) override;

    /// Enqueues loading and applying block {@param block_info} from peer
    /// {@param peer_id}.
    /// If provided block is the best after applying, {@param handler} be called
    void enqueue(const primitives::BlockHeader &header,
                 const libp2p::peer::PeerId &peer_id,
                 SyncResultHandler &&handler) override;

   protected:
    /// @returns true, if block is already enqueued for loading
    bool isInQueue(const primitives::BlockHash &hash) const;

    /// Finds best common block with peer {@param peer_id} in provided interval.
    /// It is using tail-recursive algorithm, till {@param hint} is
    /// the needed block
    /// @param lower is number of definitely known common block (i.e. last
    /// finalized).
    /// @param upper is number of definitely unknown block.
    /// @param hint is examining block for current call
    /// @param handler callback what is called at the end of proccess
    void findCommonBlock(const libp2p::peer::PeerId &peer_id,
                         primitives::BlockNumber lower,
                         primitives::BlockNumber upper,
                         primitives::BlockNumber hint,
                         SyncResultHandler &&handler) const;

    /// Loads blocks from peer {@param peer_id} since block {@param from} till
    /// its best. Calls {@param handler} when process is finished or failed
    void loadBlocks(const libp2p::peer::PeerId &peer_id,
                    primitives::BlockInfo from,
                    SyncResultHandler &&handler);

    /// Tries to request another portion of block
    void askNextPortionOfBlocks();

    /// Pops next block from queue and tries to apply that
    void applyNextBlock();

    /// Removes block {@param block} and all all dependent on it from the queue
    /// @returns number of affected blocks
    size_t discardBlock(const primitives::BlockHash &block);

    /// Removes blocks what will never be applied beause they are contained in
    /// side-branch for provided finalized block {@param finalized_block}
    void prune(const primitives::BlockInfo &finalized_block);

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<BlockExecutor> block_executor_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<crypto::Hasher> hasher_;

    log::Logger log_ =
        log::createLogger("BabeSynchronizer", "babe_synchronizer");

    bool node_is_shutting_down_ = false;

    struct KnownBlock {
      /// Data of block
      primitives::BlockData data;
      /// Peers who know this block
      std::set<libp2p::peer::PeerId> peers;
    };

    // Already known (enqueued) but is not applyed yet
    std::unordered_map<primitives::BlockHash, KnownBlock> known_blocks_;

    // Blocks grouped by number
    std::multimap<primitives::BlockNumber, primitives::BlockHash> generations_;

    // Links parent->child
    std::unordered_multimap<primitives::BlockHash, primitives::BlockHash>
        ancestry_;

    // Number of blocks that is potentially best now
    primitives::BlockNumber watched_blocks_number_{};

    // Handlers what will be called when block is apply
    std::unordered_multimap<primitives::BlockHash, SyncResultHandler>
        watched_blocks_;

    std::atomic_bool applying_in_progress_ = false;
    std::atomic_bool asking_blocks_portion_in_progress_ = false;
    std::set<libp2p::peer::PeerId> busy_peers_;
  };

}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, BabeSynchronizerImpl::Error)

#endif  //  KAGOME_CONSENSUS_BABE_SYNCHRONIZERIMPL2
