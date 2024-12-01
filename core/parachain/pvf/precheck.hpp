/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <unordered_map>

#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "parachain/validator/signer.hpp"
#include "primitives/event_types.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome {
  class PoolHandler;
}

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::offchain {
  class OffchainWorkerFactory;
  class OffchainWorkerPool;
}  // namespace kagome::offchain

namespace kagome::parachain {
  class PvfThreadPool;
}

namespace kagome::runtime {
  class Executor;
}  // namespace kagome::runtime

namespace kagome::parachain {
  class PvfPool;

  class IPvfPrecheck {
   public:
    using BroadcastCallback = std::function<void(
        const primitives::BlockHash &, const network::SignedBitfield &)>;
    using Candidates = std::vector<std::optional<network::CandidateHash>>;

    virtual ~IPvfPrecheck() = default;

    /// Subscribes to new heads.
    virtual void start() = 0;
  };

  /// Signs pvf check statement for every new head.
  class PvfPrecheck : public IPvfPrecheck, public std::enable_shared_from_this<PvfPrecheck> {
   public:
    PvfPrecheck(
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<ValidatorSignerFactory> signer_factory,
        std::shared_ptr<runtime::ParachainHost> parachain_api,
        std::shared_ptr<PvfPool> pvf_pool,
        std::shared_ptr<runtime::Executor> executor,
        PvfThreadPool &pvf_thread_pool,
        std::shared_ptr<offchain::OffchainWorkerFactory>
            offchain_worker_factory,
        std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine);

    /// Subscribes to new heads.
    void start() override;

   private:
    using BlockHash = primitives::BlockHash;

    outcome::result<void> onBlock();

    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<ValidatorSignerFactory> signer_factory_;
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<PvfPool> pvf_pool_;
    std::shared_ptr<runtime::Executor> executor_;
    std::shared_ptr<offchain::OffchainWorkerFactory> offchain_worker_factory_;
    std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool_;
    primitives::events::ChainSub chain_sub_;
    std::map<SessionIndex, std::unordered_map<ValidationCodeHash, bool>>
        session_code_accept_;
    std::shared_ptr<PoolHandler> pvf_thread_handler_;
    log::Logger logger_ = log::createLogger("PvfPrecheck", "parachain");
  };
}  // namespace kagome::parachain
