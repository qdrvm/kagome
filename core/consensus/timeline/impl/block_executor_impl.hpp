/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/block_executor.hpp"

#include <libp2p/peer/peer_id.hpp>

#include "consensus/babe/types/babe_configuration.hpp"
#include "log/logger.hpp"
#include "primitives/block_header.hpp"
#include "primitives/event_types.hpp"
#include "telemetry/service.hpp"
#include "utils/weak_io_context.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::common {
  class WorkerThreadPool;
}

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::runtime {
  class OffchainWorkerApi;
  class Core;
};  // namespace kagome::runtime

namespace kagome::transaction_pool {
  class TransactionPool;
}

namespace kagome::consensus {

  class BlockAppenderBase;

  class BlockExecutorImpl
      : public BlockExecutor,
        public std::enable_shared_from_this<BlockExecutorImpl> {
   public:
    BlockExecutorImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        const common::WorkerThreadPool &worker_thread_pool,
        WeakIoContext main_thread_context,
        std::shared_ptr<runtime::Core> core,
        std::shared_ptr<transaction_pool::TransactionPool> tx_pool,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
        primitives::events::StorageSubscriptionEnginePtr storage_sub_engine,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        std::unique_ptr<BlockAppenderBase> appender);

    ~BlockExecutorImpl();

    void applyBlock(
        primitives::Block &&block,
        const std::optional<primitives::Justification> &justification,
        ApplyJustificationCb &&callback) override;

   private:
    void applyBlockExecuted(
        primitives::Block &&block,
        const std::optional<primitives::Justification> &justification,
        ApplyJustificationCb &&callback,
        const primitives::BlockInfo &block_info,
        clock::SteadyClock::TimePoint start_time,
        const primitives::BlockInfo &previous_best_block);

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    WeakIoContext worker_thread_context_;
    WeakIoContext main_thread_context_;
    std::shared_ptr<runtime::Core> core_;
    std::shared_ptr<transaction_pool::TransactionPool> tx_pool_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api_;
    primitives::events::StorageSubscriptionEnginePtr storage_sub_engine_;
    primitives::events::ChainSubscriptionEnginePtr chain_subscription_engine_;

    std::unique_ptr<BlockAppenderBase> appender_;

    log::Logger logger_;
    telemetry::Telemetry telemetry_;
  };

}  // namespace kagome::consensus
