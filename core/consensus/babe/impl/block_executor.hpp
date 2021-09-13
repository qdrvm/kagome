/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_IMPL_BLOCK_EXECUTOR_HPP
#define KAGOME_CORE_CONSENSUS_BABE_IMPL_BLOCK_EXECUTOR_HPP

#include <libp2p/peer/peer_id.hpp>

#include "blockchain/block_tree.hpp"
#include "clock/timer.hpp"
#include "consensus/authority/authority_update_observer.hpp"
#include "consensus/babe/babe_util.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/validation/block_validator.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/block_data.hpp"
#include "primitives/block_header.hpp"
#include "runtime/runtime_api/core.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::consensus {

  class BlockExecutor : public std::enable_shared_from_this<BlockExecutor> {
   public:
    enum class Error { INVALID_BLOCK = 1, BLOCK_EXECUTOR_GONE };

    BlockExecutor(std::shared_ptr<blockchain::BlockTree> block_tree,
                  std::shared_ptr<runtime::Core> core,
                  std::shared_ptr<primitives::BabeConfiguration> configuration,
                  std::shared_ptr<BlockValidator> block_validator,
                  std::shared_ptr<grandpa::Environment> grandpa_environment,
                  std::shared_ptr<transaction_pool::TransactionPool> tx_pool,
                  std::shared_ptr<crypto::Hasher> hasher,
                  std::shared_ptr<authority::AuthorityUpdateObserver>
                      authority_update_observer,
                  std::shared_ptr<BabeUtil> babe_util,
                  std::shared_ptr<boost::asio::io_context> io_context,
                  std::unique_ptr<clock::Timer> sync_timer);


    // should only be invoked when parent of block exists
    outcome::result<void> applyBlock(const primitives::BlockData &block);

   private:
    /// Possible states of the block executor
    enum ExecutorState {
      /// Executor had been synced and ready to work.
      kReadyState = 0,

      /// Executor at the syncing state. It doesn't process blocks from the
      /// past.
      kSyncState = 1,
    };
    std::atomic<ExecutorState> sync_state_;
    std::unique_ptr<clock::Timer> sync_timer_;

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::Core> core_;
    std::shared_ptr<primitives::BabeConfiguration> babe_configuration_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<grandpa::Environment> grandpa_environment_;
    std::shared_ptr<transaction_pool::TransactionPool> tx_pool_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<authority::AuthorityUpdateObserver>
        authority_update_observer_;
    std::shared_ptr<BabeUtil> babe_util_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    log::Logger logger_;

    /**
     * Aux class for doing iterable action aynchronously (not all iteration as
     * solid execution)
     */
    class AsyncHelper : public std::enable_shared_from_this<AsyncHelper> {
     public:
      AsyncHelper() = delete;
      AsyncHelper(AsyncHelper &&) noexcept = delete;
      AsyncHelper(const AsyncHelper &) = delete;

      AsyncHelper(std::shared_ptr<boost::asio::io_context> context)
          : std::enable_shared_from_this<AsyncHelper>(),
            io_context_(std::move(context)){};

      ~AsyncHelper() = default;

      // Does next iteration
      std::function<void()> next() {
        return [self = shared_from_this()] {
          self->io_context_->post([wp = self->weak_from_this()] {
            if (auto self = wp.lock()) {
              self->func_();
            }
          });
        };
      }

      // Set up iterable function
      void setFunction(std::function<void()> &&func) {
        func_ = std::move(func);
      }

      // Start function (does first iteration)
      void run() {
        func_();
      }

     private:
      std::shared_ptr<boost::asio::io_context> io_context_;
      std::function<void()> func_;
    };  // namespace kagome::common
  };

}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, BlockExecutor::Error);

#endif  // KAGOME_CORE_CONSENSUS_BABE_IMPL_BLOCK_EXECUTOR_HPP
