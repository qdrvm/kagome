/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKEXECUTORIMPL
#define KAGOME_CONSENSUS_BLOCKEXECUTORIMPL

#include "consensus/timeline/block_executor.hpp"

#include <libp2p/peer/peer_id.hpp>

#include "log/logger.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/block_header.hpp"
#include "primitives/event_types.hpp"
#include "telemetry/service.hpp"

namespace kagome::runtime {
  class OffchainWorkerApi;
  class Core;
};  // namespace kagome::runtime

namespace kagome::blockchain {
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::transaction_pool {
  class TransactionPool;
}

namespace kagome::consensus::babe {

  class BlockAppenderBase;

  class BlockExecutorImpl
      : public BlockExecutor,
        public std::enable_shared_from_this<BlockExecutorImpl> {
   public:
    BlockExecutorImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
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
    std::shared_ptr<blockchain::BlockTree> block_tree_;
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

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BLOCKEXECUTORIMPL
