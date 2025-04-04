/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/environment.hpp"

#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "parachain/approval/approved_ancestor.hpp"

namespace kagome {
  class PoolHandler;
}  // namespace kagome

namespace kagome::application {
  class AppStateManager;
}  // namespace kagome::application

namespace kagome::blockchain {
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::common {
  class MainThreadPool;
}

namespace kagome::consensus::grandpa {
  class AuthorityManager;
}  // namespace kagome::consensus::grandpa

namespace kagome::dispute {
  class DisputeCoordinator;
}

namespace kagome::network {
  class GrandpaTransmitter;
}

namespace kagome::offchain {
  class OffchainWorkerFactory;
  class OffchainWorkerPool;
}  // namespace kagome::offchain

namespace kagome::runtime {
  class ParachainHost;
  class GrandpaApi;
}  // namespace kagome::runtime

namespace kagome::parachain {
  class BackingStore;
}

namespace kagome::consensus::grandpa {
  class IVerifiedJustificationQueue;

  class EnvironmentImpl : public Environment,
                          public std::enable_shared_from_this<EnvironmentImpl> {
   public:
    EnvironmentImpl(
        application::AppStateManager &app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<AuthorityManager> authority_manager,
        std::shared_ptr<network::GrandpaTransmitter> transmitter,
        std::shared_ptr<parachain::IApprovedAncestor> approved_ancestor,
        LazySPtr<JustificationObserver> justification_observer,
        std::shared_ptr<IVerifiedJustificationQueue>
            verified_justification_queue,
        std::shared_ptr<runtime::GrandpaApi> grandpa_api,
        std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator,
        std::shared_ptr<runtime::ParachainHost> parachain_api,
        std::shared_ptr<parachain::BackingStore> backing_store,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<offchain::OffchainWorkerFactory>
            offchain_worker_factory,
        std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool,
        common::MainThreadPool &main_thread_pool);

    ~EnvironmentImpl() override = default;

    // Chain methods

    bool hasBlock(const primitives::BlockHash &block) const override;

    outcome::result<std::vector<primitives::BlockHash>> getAncestry(
        const primitives::BlockHash &base,
        const primitives::BlockHash &block) const override;

    bool hasAncestry(const primitives::BlockHash &base,
                     const primitives::BlockHash &block) const override;

    outcome::result<BlockInfo> bestChainContaining(
        const primitives::BlockHash &base_hash,
        std::optional<VoterSetId> voter_set_id) const override;

    // Environment methods

    void onCatchUpRequested(const libp2p::peer::PeerId &peer_id,
                            VoterSetId set_id,
                            RoundNumber round_number) override;

    void onCatchUpRespond(const libp2p::peer::PeerId &peer_id,
                          VoterSetId set_id,
                          RoundNumber round_number,
                          std::vector<SignedPrevote> prevote_justification,
                          std::vector<SignedPrecommit> precommit_justification,
                          BlockInfo best_final_candidate) override;

    void sendState(const libp2p::peer::PeerId &peer_id,
                   const MovableRoundState &state,
                   VoterSetId voter_set_id) override;

    void onVoted(RoundNumber round,
                 VoterSetId set_id,
                 const SignedMessage &vote) override;

    void onCommitted(RoundNumber round,
                     VoterSetId voter_ser_id,
                     const BlockInfo &vote,
                     const GrandpaJustification &justification) override;

    void onNeighborMessageSent(RoundNumber round,
                               VoterSetId set_id,
                               BlockNumber last_finalized) override;

    void applyJustification(const BlockInfo &block_info,
                            const primitives::Justification &justification,
                            ApplyJustificationCb &&cb) override;

    outcome::result<void> finalize(
        VoterSetId id, const GrandpaJustification &justification) override;

    // Getters

    outcome::result<GrandpaJustification> getJustification(
        const BlockHash &block_hash) override;

    outcome::result<void> reportEquivocation(
        const VotingRound &round,
        const Equivocation &equivocation) const override;

    outcome::result<void> makeAncestry(
        GrandpaJustification &justification) const override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<AuthorityManager> authority_manager_;
    std::shared_ptr<network::GrandpaTransmitter> transmitter_;
    std::shared_ptr<parachain::IApprovedAncestor> approved_ancestor_;
    LazySPtr<JustificationObserver> justification_observer_;
    std::shared_ptr<IVerifiedJustificationQueue> verified_justification_queue_;
    std::shared_ptr<runtime::GrandpaApi> grandpa_api_;
    std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator_;
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<parachain::BackingStore> backing_store_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<offchain::OffchainWorkerFactory> offchain_worker_factory_;
    std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool_;
    std::shared_ptr<PoolHandler> main_pool_handler_;

    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_approval_lag_;

    log::Logger logger_;
  };

}  // namespace kagome::consensus::grandpa
