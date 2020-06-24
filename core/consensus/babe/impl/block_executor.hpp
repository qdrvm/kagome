/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_IMPL_BLOCK_EXECUTOR_HPP
#define KAGOME_CORE_CONSENSUS_BABE_IMPL_BLOCK_EXECUTOR_HPP

#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/babe/babe_synchronizer.hpp"
#include "consensus/babe/epoch_storage.hpp"
#include "consensus/validation/block_validator.hpp"
#include "crypto/hasher.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/block_header.hpp"
#include "runtime/core.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::consensus {

  class BlockExecutor : public std::enable_shared_from_this<BlockExecutor> {
   public:
    BlockExecutor(std::shared_ptr<blockchain::BlockTree> block_tree,
                  std::shared_ptr<runtime::Core> core,
                  std::shared_ptr<primitives::BabeConfiguration> configuration,
                  std::shared_ptr<BabeSynchronizer> babe_synchronizer,
                  std::shared_ptr<BlockValidator> block_validator,
                  std::shared_ptr<EpochStorage> epoch_storage,
                  std::shared_ptr<transaction_pool::TransactionPool> tx_pool,
                  std::shared_ptr<crypto::Hasher> hasher);

    /**
     * Processes next header: if header is observed first it is added to the
     * storage, handler is invoked. Synchronization of blocks between new one
     * and the current best one is launched if required
     * @param header new header that we received and trying to process
     * @param new_block_handler invoked on new header if it is observed first
     * time
     */
    void processNextBlock(
        const primitives::BlockHeader &header,
        const std::function<void(const primitives::BlockHeader &)>
            &new_block_handler);

    /**
     * Synchronize all missing blocks between the last finalized and the new one
     * @param new_header header defining new block
     * @param next action after the sync is done
     */
    void requestBlocks(const primitives::BlockHeader &new_header,
                       std::function<void()> &&next);

    /**
     * Synchronize all missing blocks between provided blocks (from and to)
     * @param from starting block of syncing blocks
     * @param to last block of syncing block
     * @param next action after the sync is done
     */
    void requestBlocks(const primitives::BlockId &from,
                       const primitives::BlockHash &to,
                       primitives::AuthorityIndex authority_index,
                       std::function<void()> &&next);

   private:
    // should only be invoked when parent of block exists
    outcome::result<void> applyBlock(const primitives::Block &block);

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::Core> core_;
    std::shared_ptr<primitives::BabeConfiguration> genesis_configuration_;
    std::shared_ptr<BabeSynchronizer> babe_synchronizer_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<EpochStorage> epoch_storage_;
    std::shared_ptr<transaction_pool::TransactionPool> tx_pool_;
    std::shared_ptr<crypto::Hasher> hasher_;

    common::Logger logger_;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_BABE_IMPL_BLOCK_EXECUTOR_HPP
