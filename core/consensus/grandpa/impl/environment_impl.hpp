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
    using OnCompleted = boost::signals2::signal<void(const CompletedRound &)>;
    using OnCompletedSlotType = OnCompleted::slot_type;

   public:
    enum class Error { BLOCK_AFTER_LIMIT = 1 };

    EnvironmentImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repository,
        std::shared_ptr<Gossiper> gossiper);

    ~EnvironmentImpl() override = default;

    // Chain methods

    outcome::result<std::vector<primitives::BlockHash>> getAncestry(
        const primitives::BlockHash &base,
        const primitives::BlockHash &block) const override;

    outcome::result<BlockInfo> bestChainContaining(
        const primitives::BlockHash &base) const override;

    // Environment methods

    outcome::result<void> proposed(
        RoundNumber round,
        MembershipCounter set_id,
        const SignedPrimaryPropose &propose) override;

    outcome::result<void> prevoted(RoundNumber round,
                                   MembershipCounter set_id,
                                   const SignedPrevote &prevote) override;

    outcome::result<void> precommitted(
        RoundNumber round,
        MembershipCounter set_id,
        const SignedPrecommit &precommit) override;

    outcome::result<void> commit(
        RoundNumber round,
        const BlockInfo &vote,
        const GrandpaJustification &justification) override;

    void onCompleted(std::function<void(const CompletedRound &)>) override;

    void completed(CompletedRound round) override;

    outcome::result<void> finalize(
        const primitives::BlockHash &block_hash,
        const GrandpaJustification &justification) override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repository_;
    std::shared_ptr<Gossiper> gossiper_;

    OnCompleted on_completed_;
    common::Logger logger_;
  };

}  // namespace kagome::consensus::grandpa

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, EnvironmentImpl::Error);

#endif  // KAGOME_CHAIN_IMPL_HPP
