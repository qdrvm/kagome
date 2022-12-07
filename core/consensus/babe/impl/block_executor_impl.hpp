/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKEXECUTORIMPL
#define KAGOME_CONSENSUS_BLOCKEXECUTORIMPL

#include "consensus/babe/block_executor.hpp"

#include <libp2p/peer/peer_id.hpp>

#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/block_header.hpp"
#include "telemetry/service.hpp"

namespace kagome::runtime {
  class OffchainWorkerApi;
  class Core;
};  // namespace kagome::runtime

namespace kagome::consensus::babe {
  class BabeConfigRepository;
  class BabeUtil;
  class BlockValidator;
  class ConsistencyKeeper;
}  // namespace kagome::consensus::babe

namespace kagome::consensus::grandpa {
  class Environment;
}

namespace kagome::blockchain {
  class DigestTracker;
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::transaction_pool {
  class TransactionPool;
}

namespace kagome::consensus::babe {

  class BlockExecutorImpl
      : public BlockExecutor,
        public std::enable_shared_from_this<BlockExecutorImpl> {
   public:
    enum class Error { INVALID_BLOCK = 1, PARENT_NOT_FOUND, INTERNAL_ERROR };

    BlockExecutorImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<runtime::Core> core,
        std::shared_ptr<BabeConfigRepository> babe_config_repo,
        std::shared_ptr<BlockValidator> block_validator,
        std::shared_ptr<grandpa::Environment> grandpa_environment,
        std::shared_ptr<transaction_pool::TransactionPool> tx_pool,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<blockchain::DigestTracker> digest_tracker,
        std::shared_ptr<BabeUtil> babe_util,
        std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
        std::shared_ptr<ConsistencyKeeper> consistency_keeper);

    outcome::result<void> applyBlock(primitives::BlockData &&block) override;

    outcome::result<void> applyJustification(
        const primitives::BlockInfo &block_info,
        const primitives::Justification &justification) override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::Core> core_;
    std::shared_ptr<BabeConfigRepository> babe_config_repo_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<grandpa::Environment> grandpa_environment_;
    std::shared_ptr<transaction_pool::TransactionPool> tx_pool_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<blockchain::DigestTracker> digest_tracker_;
    std::shared_ptr<BabeUtil> babe_util_;
    std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api_;
    std::shared_ptr<ConsistencyKeeper> consistency_keeper_;

    // Justification Store for Future Applying
    std::map<primitives::BlockInfo, primitives::Justification>
        postponed_justifications_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Histogram *metric_block_execution_time_;

    log::Logger logger_;
    telemetry::Telemetry telemetry_;
  };

}  // namespace kagome::consensus::babe

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::babe, BlockExecutorImpl::Error);

#endif  // KAGOME_CONSENSUS_BLOCKEXECUTORIMPL
