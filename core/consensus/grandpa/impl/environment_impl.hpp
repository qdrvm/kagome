/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
        boost::signals2::signal<void(outcome::result<CompletedRound>)>;
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

    outcome::result<BlockInfo> bestChainContaining(
        const primitives::BlockHash &base) const override;

    // Environment methods

    outcome::result<void> onProposed(
        RoundNumber round,
        MembershipCounter set_id,
        const SignedPrimaryPropose &propose) override;

    outcome::result<void> onPrevoted(RoundNumber round,
                                     MembershipCounter set_id,
                                     const SignedPrevote &prevote) override;

    outcome::result<void> onPrecommitted(
        RoundNumber round,
        MembershipCounter set_id,
        const SignedPrecommit &precommit) override;

    outcome::result<void> onCommitted(
        RoundNumber round,
        const BlockInfo &vote,
        const GrandpaJustification &justification) override;

    void doOnCompleted(const CompleteHandler &) override;

    void onCompleted(outcome::result<CompletedRound> round) override;

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

#endif  // KAGOME_CHAIN_IMPL_HPP
