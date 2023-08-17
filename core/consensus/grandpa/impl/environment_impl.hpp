/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_ENVIRONMENTIMPL
#define KAGOME_CONSENSUS_GRANDPA_ENVIRONMENTIMPL

#include "consensus/grandpa/environment.hpp"

#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "parachain/approval/approved_ancestor.hpp"
#include "utils/thread_pool.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::dispute {
  class DisputeCoordinator;
}

namespace kagome::consensus::grandpa {
  class AuthorityManager;
}

namespace kagome::network {
  class GrandpaTransmitter;
}

namespace kagome::runtime {
  class ParachainHost;
}

namespace kagome::parachain {
  class BackingStore;
}

namespace kagome::consensus::grandpa {

  class EnvironmentImpl : public Environment,
                          public std::enable_shared_from_this<EnvironmentImpl> {
   public:
    EnvironmentImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repository,
        std::shared_ptr<AuthorityManager> authority_manager,
        std::shared_ptr<network::GrandpaTransmitter> transmitter,
        std::shared_ptr<parachain::IApprovedAncestor> approved_ancestor,
        LazySPtr<JustificationObserver> justification_observer,
        std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator,
        std::shared_ptr<runtime::ParachainHost> parachain_api,
        std::shared_ptr<parachain::BackingStore> backing_store,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<boost::asio::io_context> main_thread_context);

    ~EnvironmentImpl() override = default;

    // Chain methods

    outcome::result<bool> hasBlock(
        const primitives::BlockHash &block) const override;

    outcome::result<std::vector<primitives::BlockHash>> getAncestry(
        const primitives::BlockHash &base,
        const primitives::BlockHash &block) const override;

    bool hasAncestry(const primitives::BlockHash &base,
                     const primitives::BlockHash &block) const override;

    outcome::result<BlockInfo> bestChainContaining(
        const primitives::BlockHash &base,
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

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repository_;
    std::shared_ptr<AuthorityManager> authority_manager_;
    std::shared_ptr<network::GrandpaTransmitter> transmitter_;
    std::shared_ptr<parachain::IApprovedAncestor> approved_ancestor_;
    LazySPtr<JustificationObserver> justification_observer_;
    std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator_;
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<parachain::BackingStore> backing_store_;
    std::shared_ptr<crypto::Hasher> hasher_;
    ThreadHandler main_thread_context_;

    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_approval_lag_;

    log::Logger logger_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_ENVIRONMENTIMPL
