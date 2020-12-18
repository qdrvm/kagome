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
    // if base equal to block, then return list with single block
    if (base == block) {
      return std::vector<BlockHash>{base};
    }

    OUTCOME_TRY(chain, block_tree_->getChainByBlocks(base, block));
    std::reverse(chain.begin(), chain.end());
    return std::move(chain);
  }

  bool EnvironmentImpl::hasAncestry(const BlockHash &base,
                                    const BlockHash &block) const {
    return base == block || block_tree_->hasDirectChain(base, block);
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

  outcome::result<void> EnvironmentImpl::onCatchUpRequested(
      const libp2p::peer::PeerId &peer_id,
      MembershipCounter set_id,
      RoundNumber round_number) {
    network::CatchUpRequest message{.round_number = round_number,
                                    .voter_set_id = set_id};
    gossiper_->catchUpRequest(peer_id, message);
    logger_->debug("Catch-Up-Request sent from round #{}", round_number);
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onCatchUpResponsed(
      const libp2p::peer::PeerId &peer_id,
      MembershipCounter set_id,
      RoundNumber round_number,
      GrandpaJustification prevote_justification,
      GrandpaJustification precommit_justification,
      BlockInfo best_final_candidate) {
    network::CatchUpResponse message{
        .voter_set_id = set_id,
        .round_number = round_number,
        .prevote_justification = std::move(prevote_justification),
        .precommit_justification = std::move(precommit_justification),
        .best_final_candidate = best_final_candidate};
    gossiper_->catchUpResponse(peer_id, message);
    logger_->debug("Catch-Up-Response sent upto round {}", round_number);
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onProposed(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedMessage &propose) {
    BOOST_ASSERT(propose.is<PrimaryPropose>());
    network::GrandpaVoteMessage message{
        {.round_number = round, .counter = set_id, .vote = propose}};
    gossiper_->vote(message);
    logger_->debug("Round #{}: Proposed block #{} with hash {}",
                   round,
                   propose.block_number(),
                   propose.block_hash().toHex());
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onPrevoted(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedMessage &prevote) {
    BOOST_ASSERT(prevote.is<Prevote>());
    network::GrandpaVoteMessage message{
        {.round_number = round, .counter = set_id, .vote = prevote}};
    gossiper_->vote(message);
    logger_->debug("Round #{}: Prevoted block #{} with hash {}",
                   round,
                   prevote.block_number(),
                   prevote.block_hash().toHex());
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onPrecommitted(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedMessage &precommit) {
    BOOST_ASSERT(precommit.is<Precommit>());
    network::GrandpaVoteMessage message{
        {.round_number = round, .counter = set_id, .vote = precommit}};
    gossiper_->vote(message);
    logger_->debug("Round #{}: Precommitted block #{} with hash {}",
                   round,
                   precommit.block_number(),
                   precommit.block_hash().toHex());
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onCommitted(
      RoundNumber round,
      const BlockInfo &vote,
      const GrandpaJustification &justification) {
    logger_->debug("Round #{}: Committed block #{} with hash {}",
                   round,
                   vote.block_number,
                   vote.block_hash.toHex());
    network::GrandpaPreCommit message{
        {.round_number = round, .vote = vote, .justification = justification}};
    gossiper_->finalize(message);
    return outcome::success();
  }

  void EnvironmentImpl::doOnCompleted(
      const CompleteHandler &on_completed_slot) {
    on_completed_.disconnect_all_slots();
    on_completed_.connect(on_completed_slot);
  }

  void EnvironmentImpl::onCompleted(
      outcome::result<MovableRoundState> round_state) {
    BOOST_ASSERT_MSG(
        not on_completed_.empty(),
        "Completed signal in environment cannot be empty when it is invoked");
    on_completed_(round_state);
  }

  outcome::result<void> EnvironmentImpl::finalize(
      const primitives::BlockHash &block_hash,
      const GrandpaJustification &grandpa_jusitification) {
    primitives::Justification justification;
    justification.data.put(scale::encode(grandpa_jusitification).value());
    return block_tree_->finalize(block_hash, justification);
  }

  outcome::result<GrandpaJustification> EnvironmentImpl::getJustification(
      const BlockHash &block_hash) {
    OUTCOME_TRY(encoded_justification,
                block_tree_->getBlockJustification(block_hash));

    OUTCOME_TRY(
        grandpa_jusitification,
        scale::decode<GrandpaJustification>(encoded_justification.data));

    return outcome::success(std::move(grandpa_jusitification));
  }

}  // namespace kagome::consensus::grandpa
