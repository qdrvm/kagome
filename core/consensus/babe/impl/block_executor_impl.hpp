/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKEXECUTORIMPL
#define KAGOME_CONSENSUS_BLOCKEXECUTORIMPL

#include "consensus/babe/block_executor.hpp"

#include <libp2p/peer/peer_id.hpp>

#include "blockchain/block_tree.hpp"
#include "clock/timer.hpp"
#include "consensus/babe/babe_util.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/validation/block_validator.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/block_header.hpp"
#include "runtime/runtime_api/core.hpp"
#include "telemetry/service.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::runtime {
  class OffchainWorkerApi;
};

namespace kagome::consensus::babe {
  class BabeConfigRepository;
  class ConsistencyKeeper;
}  // namespace kagome::consensus::babe

namespace kagome::blockchain {
  class DigestTracker;
}

namespace kagome::consensus {

  class BlockExecutorImpl
      : public BlockExecutor,
        public std::enable_shared_from_this<BlockExecutorImpl> {
   public:
    enum class Error { INVALID_BLOCK = 1, PARENT_NOT_FOUND, INTERNAL_ERROR };

    BlockExecutorImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<runtime::Core> core,
        std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
        std::shared_ptr<BlockValidator> block_validator,
        std::shared_ptr<grandpa::Environment> grandpa_environment,
        std::shared_ptr<transaction_pool::TransactionPool> tx_pool,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<blockchain::DigestTracker> digest_tracker,
        std::shared_ptr<BabeUtil> babe_util,
        std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
        std::shared_ptr<babe::ConsistencyKeeper> consistency_keeper);

    outcome::result<void> applyBlock(primitives::BlockData &&block) override;

    outcome::result<void> applyJustification(
        const primitives::BlockInfo &block_info,
        const primitives::Justification &justification) override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::Core> core_;
    std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<grandpa::Environment> grandpa_environment_;
    std::shared_ptr<transaction_pool::TransactionPool> tx_pool_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<blockchain::DigestTracker> digest_tracker_;
    std::shared_ptr<BabeUtil> babe_util_;
    std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api_;
    std::shared_ptr<babe::ConsistencyKeeper> consistency_keeper_;

    // Justification Store for Future Applying
    std::map<primitives::BlockInfo, primitives::Justification>
        postponed_justifications_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Histogram *metric_block_execution_time_;

    log::Logger logger_;
    telemetry::Telemetry telemetry_;
  };

}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, BlockExecutorImpl::Error);

#endif  // KAGOME_CONSENSUS_BLOCKEXECUTORIMPL
