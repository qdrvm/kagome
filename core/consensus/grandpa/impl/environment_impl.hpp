/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_ENVIRONMENTIMPL
#define KAGOME_CONSENSUS_GRANDPA_ENVIRONMENTIMPL

#include "consensus/grandpa/environment.hpp"

#include "log/logger.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::consensus::grandpa {
  class AuthorityManager;
}

namespace kagome::network {
  class GrandpaTransmitter;
}

namespace kagome::consensus::grandpa {

  class EnvironmentImpl : public Environment {
   public:
    EnvironmentImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repository,
        std::shared_ptr<AuthorityManager> authority_manager,
        std::shared_ptr<network::GrandpaTransmitter> transmitter);

    ~EnvironmentImpl() override = default;

    // Back link to Grandpa

    void setJustificationObserver(
        std::weak_ptr<JustificationObserver> justification_observer) override {
      BOOST_ASSERT(justification_observer_.expired());
      justification_observer_ = std::move(justification_observer);
    }

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

    outcome::result<void> onCatchUpRequested(
        const libp2p::peer::PeerId &peer_id,
        VoterSetId set_id,
        RoundNumber round_number) override;

    outcome::result<void> onCatchUpRespond(
        const libp2p::peer::PeerId &peer_id,
        VoterSetId set_id,
        RoundNumber round_number,
        std::vector<SignedPrevote> prevote_justification,
        std::vector<SignedPrecommit> precommit_justification,
        BlockInfo best_final_candidate) override;

    void sendState(const libp2p::peer::PeerId &peer_id,
                   const MovableRoundState &state,
                   VoterSetId voter_set_id) override;

    outcome::result<void> onVoted(RoundNumber round,
                                  VoterSetId set_id,
                                  const SignedMessage &vote) override;

    outcome::result<void> onCommitted(
        RoundNumber round,
        VoterSetId voter_ser_id,
        const BlockInfo &vote,
        const GrandpaJustification &justification) override;

    outcome::result<void> onNeighborMessageSent(
        RoundNumber round,
        VoterSetId set_id,
        BlockNumber last_finalized) override;

    outcome::result<void> applyJustification(
        const BlockInfo &block_info,
        const primitives::Justification &justification) override;

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
    std::weak_ptr<JustificationObserver> justification_observer_;

    log::Logger logger_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_ENVIRONMENTIMPL
