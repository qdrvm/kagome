/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_IMPL_HPP
#define KAGOME_CHAIN_IMPL_HPP

#include "consensus/grandpa/environment.hpp"

#include <boost/signals2/signal.hpp>

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/grandpa/chain.hpp"
#include "consensus/grandpa/gossiper.hpp"

namespace kagome::consensus::grandpa {

  class EnvironmentImpl : public Environment {
    using OnCompleted =
        boost::signals2::signal<void(outcome::result<MovableRoundState>)>;
    using OnCompletedSlotType = OnCompleted::slot_type;

   public:
    EnvironmentImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repository,
        std::shared_ptr<Gossiper> gossiper);

    ~EnvironmentImpl() override = default;

    // Chain methods

    outcome::result<std::vector<primitives::BlockHash>> getAncestry(
        const primitives::BlockHash &base,
        const primitives::BlockHash &block) const override;

    bool hasAncestry(const primitives::BlockHash &base,
                     const primitives::BlockHash &block) const override;

    outcome::result<BlockInfo> bestChainContaining(
        const primitives::BlockHash &base) const override;

    // Environment methods

    outcome::result<void> onCatchUpRequested(
        const libp2p::peer::PeerId &peer_id,
        MembershipCounter set_id,
        RoundNumber round_number) override;

    outcome::result<void> onCatchUpResponsed(
        const libp2p::peer::PeerId &peer_id,
        MembershipCounter set_id,
        RoundNumber round_number,
        GrandpaJustification prevote_justification,
        GrandpaJustification precommit_justification,
        BlockInfo best_final_candidate) override;

    outcome::result<void> onProposed(RoundNumber round,
                                     MembershipCounter set_id,
                                     const SignedMessage &propose) override;

    outcome::result<void> onPrevoted(RoundNumber round,
                                     MembershipCounter set_id,
                                     const SignedMessage &prevote) override;

    outcome::result<void> onPrecommitted(
        RoundNumber round,
        MembershipCounter set_id,
        const SignedMessage &precommit) override;

    outcome::result<void> onCommitted(
        RoundNumber round,
        const BlockInfo &vote,
        const GrandpaJustification &justification) override;

    void doOnCompleted(const CompleteHandler &) override;

    void onCompleted(outcome::result<MovableRoundState> round) override;

    outcome::result<void> finalize(
        const primitives::BlockHash &block_hash,
        const GrandpaJustification &justification) override;

    // Getters

    outcome::result<GrandpaJustification> getJustification(
        const BlockHash &block_hash) override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repository_;
    std::shared_ptr<Gossiper> gossiper_;

    OnCompleted on_completed_;
    common::Logger logger_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CHAIN_IMPL_HPP
