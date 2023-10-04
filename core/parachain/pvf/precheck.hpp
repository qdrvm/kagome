/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_PVF_PRECHECK_HPP
#define KAGOME_PARACHAIN_PVF_PRECHECK_HPP

#include <unordered_set>

#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "parachain/validator/signer.hpp"
#include "primitives/event_types.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "utils/thread_pool.hpp"

namespace kagome::offchain {
  class OffchainWorkerFactory;
  class OffchainWorkerPool;
}  // namespace kagome::offchain

namespace kagome::runtime {
  class Executor;
  class ModuleFactory;
}  // namespace kagome::runtime

namespace kagome::parachain {
  /// Signs pvf check statement for every new head.
  class PvfPrecheck : public std::enable_shared_from_this<PvfPrecheck> {
   public:
    using BroadcastCallback = std::function<void(
        const primitives::BlockHash &, const network::SignedBitfield &)>;
    using Candidates = std::vector<std::optional<network::CandidateHash>>;

    PvfPrecheck(
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<ValidatorSignerFactory> signer_factory,
        std::shared_ptr<runtime::ParachainHost> parachain_api,
        std::shared_ptr<runtime::ModuleFactory> module_factory,
        std::shared_ptr<runtime::Executor> executor,
        std::shared_ptr<offchain::OffchainWorkerFactory>
            offchain_worker_factory,
        std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool);

    /// Subscribes to new heads.
    void start(std::shared_ptr<primitives::events::ChainSubscriptionEngine>
                   chain_sub_engine);

   private:
    using BlockHash = primitives::BlockHash;

    outcome::result<void> onBlock(const BlockHash &block_hash,
                                  const primitives::BlockHeader &header);

    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<ValidatorSignerFactory> signer_factory_;
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<runtime::ModuleFactory> module_factory_;
    std::shared_ptr<runtime::Executor> executor_;
    std::shared_ptr<offchain::OffchainWorkerFactory> offchain_worker_factory_;
    std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;
    std::unordered_set<ValidationCodeHash> seen_;
    std::shared_ptr<ThreadPool> thread_{ThreadPool::create("PvfPrecheck", 1)};
    log::Logger logger_ = log::createLogger("PvfPrecheck", "parachain");
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_PVF_PRECHECK_HPP
