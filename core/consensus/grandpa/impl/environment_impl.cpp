/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/environment_impl.hpp"

#include <utility>

#include <boost/optional/optional_io.hpp>
#include "scale/scale.hpp"

namespace kagome::consensus::grandpa {

  using primitives::BlockHash;
  using primitives::BlockNumber;

  EnvironmentImpl::EnvironmentImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repository,
      std::shared_ptr<Gossiper> gossiper)
      : block_tree_{std::move(block_tree)},
        header_repository_{std::move(header_repository)},
        gossiper_{std::move(gossiper)},
        logger_{common::createLogger("Grandpa environment:")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(header_repository_ != nullptr);
    BOOST_ASSERT(gossiper_ != nullptr);
  }

  outcome::result<std::vector<BlockHash>> EnvironmentImpl::getAncestry(
      const BlockHash &base, const BlockHash &block) const {
    // if base equal to block, then return empty list
    if (base == block) {
      return std::vector<BlockHash>{};
    }
    OUTCOME_TRY(chain, block_tree_->getChainByBlocks(base, block));
    std::vector<BlockHash> result_chain(chain.size() - 2);
    std::move(chain.rbegin() + 1, chain.rend() - 1, result_chain.begin());
    return result_chain;
  }

  outcome::result<BlockInfo> EnvironmentImpl::bestChainContaining(
      const BlockHash &base) const {
    logger_->debug("Finding best chain containing block {}", base.toHex());
    OUTCOME_TRY(best_info, block_tree_->getBestContaining(base, boost::none));
    auto best_hash = best_info.block_hash;

    auto target = best_info.block_number;

    OUTCOME_TRY(best_header, header_repository_->getBlockHeader(best_hash));

    // walk backwards until we find the target block
    while (true) {
      if (best_header.number == target) {
        logger_->debug("found best chain: number {}, hash {}",
                       best_header.number,
                       best_hash.toHex());
        return BlockInfo{primitives::BlockNumber{best_header.number},
                         best_hash};
      }
      best_hash = best_header.parent_hash;
      OUTCOME_TRY(new_best_header,
                  header_repository_->getBlockHeader(best_hash));
      best_header = new_best_header;
    }
  }

  outcome::result<void> EnvironmentImpl::onProposed(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedMessage &propose) {
    BOOST_ASSERT(propose.is<PrimaryPropose>());
    VoteMessage message{
        .round_number = round, .counter = set_id, .vote = propose};
    gossiper_->vote(message);
    logger_->debug("Primary proposed block with hash {} in grandpa round {}",
                   propose.block_hash().toHex(),
                   round);
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onPrevoted(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedMessage &prevote) {
    BOOST_ASSERT(prevote.is<Prevote>());
    VoteMessage message{
        .round_number = round, .counter = set_id, .vote = prevote};
    gossiper_->vote(message);
    logger_->debug("Prevoted block with hash {} in grandpa round {}",
                   prevote.block_hash().toHex(),
                   round);
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onPrecommitted(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedMessage &precommit) {
    BOOST_ASSERT(precommit.is<Precommit>());
    VoteMessage message{
        .round_number = round, .counter = set_id, .vote = precommit};
    gossiper_->vote(message);
    logger_->debug("Precommitted block with hash {} in grandpa round {}",
                   precommit.block_hash().toHex(),
                   round);
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onCommitted(
      RoundNumber round,
      const BlockInfo &vote,
      const GrandpaJustification &justification) {
    logger_->debug("Committed block with hash: {} with number: {}",
                   vote.block_hash,
                   vote.block_number);
    gossiper_->finalize(Fin{
        .round_number = round, .vote = vote, .justification = justification});
    return outcome::success();
  }

  void EnvironmentImpl::doOnCompleted(
      const CompleteHandler &on_completed_slot) {
    on_completed_.disconnect_all_slots();
    on_completed_.connect(on_completed_slot);
  }

  void EnvironmentImpl::onCompleted(outcome::result<CompletedRound> round) {
    BOOST_ASSERT_MSG(
        not on_completed_.empty(),
        "Completed signal in environment cannot be empty when it is invoked");
    on_completed_(round);
  }

  outcome::result<void> EnvironmentImpl::finalize(
      const primitives::BlockHash &block_hash,
      const GrandpaJustification &grandpa_jusitification) {
    primitives::Justification justification;
    justification.data.put(scale::encode(grandpa_jusitification).value());
    return block_tree_->finalize(block_hash, justification);
  }

}  // namespace kagome::consensus::grandpa
