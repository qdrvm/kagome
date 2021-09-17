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
#include "consensus/babe/impl/block_executor.hpp"
#include "network/router.hpp"

namespace kagome::consensus {

  class BabeSynchronizerImpl
      : public BabeSynchronizer,
        public std::enable_shared_from_this<BabeSynchronizerImpl> {
   public:
    enum class Error {
      SHUTTING_DOWN = 1,
      EMPTY_RESPONSE,
      WRONG_CONTENT,
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

    void enqueue(const primitives::BlockInfo &block_info,
                 const libp2p::peer::PeerId &peer_id,
                 SyncResultHandler &&handler) override;

    void enqueue(const primitives::BlockHeader &header,
                 const libp2p::peer::PeerId &peer_id,
                 SyncResultHandler &&handler) override;

   private:
    bool isInQueue(const primitives::BlockHash &hash) const;

    void findCommonBlock(const libp2p::peer::PeerId &peer_id,
                         primitives::BlockNumber lower,
                         primitives::BlockNumber upper,
                         primitives::BlockNumber hint,
                         SyncResultHandler &&handler) const;

    void loadBlocks(const libp2p::peer::PeerId &peer_id,
                    primitives::BlockInfo from,
                    SyncResultHandler &&handler);

    void askNextPortionOfBlocks();
    void applyNextBlock();
    size_t discardBlock(const primitives::BlockHash &block);
    void prune(const primitives::BlockInfo &finalized_block);

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<BlockExecutor> block_executor_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<crypto::Hasher> hasher_;

    log::Logger log_ =
        log::createLogger("BabeSynchronizer", "babe_synchronizer");

    bool stops_ = false;

    struct KnownBlock {
      primitives::BlockData data;
      std::set<libp2p::peer::PeerId> peers;
    };

    std::map<primitives::BlockHash, KnownBlock> known_blocks_;
    std::multimap<primitives::BlockNumber, primitives::BlockHash> generations_;
    std::multimap<primitives::BlockHash, primitives::BlockHash> ancestry_;

    primitives::BlockNumber watching_block_number_{};
    std::multimap<primitives::BlockHash, SyncResultHandler> watching_blocks_;

    std::atomic_bool applying_in_progress_ = false;
    std::atomic_bool asking_blocks_portion_in_progress_ = false;
    std::set<libp2p::peer::PeerId> busy_peers_;
  };

}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, BabeSynchronizerImpl::Error)

#endif  //  KAGOME_CONSENSUS_BABE_SYNCHRONIZERIMPL2
