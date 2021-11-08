/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/environment_impl.hpp"

#include <boost/optional/optional_io.hpp>
#include <utility>

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/grandpa/justification_observer.hpp"
#include "network/grandpa_transmitter.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus::grandpa {

  using primitives::BlockHash;
  using primitives::BlockNumber;
  using primitives::Justification;

  EnvironmentImpl::EnvironmentImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repository,
      std::shared_ptr<network::GrandpaTransmitter> transmitter)
      : block_tree_{std::move(block_tree)},
        header_repository_{std::move(header_repository)},
        transmitter_{std::move(transmitter)},
        logger_{log::createLogger("GrandpaEnvironment", "grandpa")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(header_repository_ != nullptr);
    BOOST_ASSERT(transmitter_ != nullptr);
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
    SL_DEBUG(logger_, "Finding best chain containing block {}", base.toHex());
    OUTCOME_TRY(best_info, block_tree_->getBestContaining(base, std::nullopt));
    auto best_hash = best_info.hash;

    auto target = best_info.number;

    OUTCOME_TRY(best_header, header_repository_->getBlockHeader(best_hash));

    // walk backwards until we find the target block
    while (true) {
      if (best_header.number == target) {
        SL_DEBUG(logger_,
                 "found best chain: number {}, hash {}",
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
    SL_DEBUG(
        logger_, "Send Catch-Up-Request beginning with round {}", round_number);
    network::CatchUpRequest message{.round_number = round_number,
                                    .voter_set_id = set_id};
    transmitter_->catchUpRequest(peer_id, std::move(message));
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onCatchUpResponsed(
      const libp2p::peer::PeerId &peer_id,
      MembershipCounter set_id,
      RoundNumber round_number,
      std::vector<SignedPrevote> prevote_justification,
      std::vector<SignedPrecommit> precommit_justification,
      BlockInfo best_final_candidate) {
    SL_DEBUG(logger_, "Send Catch-Up-Response upto round {}", round_number);
    network::CatchUpResponse message{
        .voter_set_id = set_id,
        .round_number = round_number,
        .prevote_justification = std::move(prevote_justification),
        .precommit_justification = std::move(precommit_justification),
        .best_final_candidate = best_final_candidate};
    transmitter_->catchUpResponse(peer_id, std::move(message));
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onProposed(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedMessage &propose) {
    BOOST_ASSERT(propose.is<PrimaryPropose>());

    SL_DEBUG(logger_,
             "Round #{}: Send proposal for block #{} with hash {}",
             round,
             propose.getBlockNumber(),
             propose.getBlockHash().toHex());

    network::GrandpaVote message{
        {.round_number = round, .counter = set_id, .vote = propose}};
    transmitter_->vote(std::move(message));
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onPrevoted(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedMessage &prevote) {
    BOOST_ASSERT(prevote.is<Prevote>());

    SL_DEBUG(logger_,
             "Round #{}: Send prevote for block #{} with hash {}",
             round,
             prevote.getBlockNumber(),
             prevote.getBlockHash().toHex());

    network::GrandpaVote message{
        {.round_number = round, .counter = set_id, .vote = prevote}};
    transmitter_->vote(std::move(message));

    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onPrecommitted(
      RoundNumber round,
      MembershipCounter set_id,
      const SignedMessage &precommit) {
    BOOST_ASSERT(precommit.is<Precommit>());

    SL_DEBUG(logger_,
             "Round #{}: Send precommit for block #{} with hash {}",
             round,
             precommit.getBlockNumber(),
             precommit.getBlockHash().toHex());

    network::GrandpaVote message{
        {.round_number = round, .counter = set_id, .vote = precommit}};
    transmitter_->vote(std::move(message));

    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onCommitted(
      RoundNumber round,
      const BlockInfo &vote,
      const GrandpaJustification &justification) {
    SL_DEBUG(logger_,
             "Round #{}: Send commit of block #{} with hash {}",
             round,
             vote.number,
             vote.hash.toHex());

    network::FullCommitMessage message{
        .round = round,
        .message = {.target_hash = vote.hash, .target_number = vote.number}};
    for (const auto &item : justification.items) {
      BOOST_ASSERT(item.is<Precommit>());
      const auto &precommit = boost::relaxed_get<Precommit>(item.message);
      message.message.precommits.push_back(precommit);
      message.message.auth_data.emplace_back(
          std::make_pair(item.signature, item.id));
    }
    transmitter_->finalize(std::move(message));

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

  outcome::result<void> EnvironmentImpl::applyJustification(
      const BlockInfo &block_info,
      const primitives::Justification &raw_justification) {
    auto justification_observer = justification_observer_.lock();
    BOOST_ASSERT(justification_observer);

    OUTCOME_TRY(
        justification,
        scale::decode<grandpa::GrandpaJustification>(raw_justification.data));

    SL_DEBUG(
        logger_,
        "Trying to apply justification on round #{} for block #{} with hash {}",
        justification.round_number,
        justification.block_info.number,
        justification.block_info.hash.toHex());

    OUTCOME_TRY(
        justification_observer->applyJustification(block_info, justification));

    return outcome::success();
  }

  bool EnvironmentImpl::contains(
      const primitives::BlockHash &block_hash) const {
    return block_tree_->contains(block_hash);
  }

  outcome::result<void> EnvironmentImpl::finalize(
      MembershipCounter id, const GrandpaJustification &grandpa_justification) {
    primitives::Justification justification;
    OUTCOME_TRY(enc, scale::encode(grandpa_justification));
    justification.data.put(enc);
    auto res = block_tree_->finalize(grandpa_justification.block_info.hash,
                                     justification);
    if (res.has_value()) {
      transmitter_->neighbor(network::GrandpaNeighborMessage{
          .round_number = grandpa_justification.round_number,
          .voter_set_id = id,
          .last_finalized = grandpa_justification.block_info.number});
    }
    return res;
  }

  outcome::result<GrandpaJustification> EnvironmentImpl::getJustification(
      const BlockHash &block_hash) {
    OUTCOME_TRY(encoded_justification,
                block_tree_->getBlockJustification(block_hash));

    OUTCOME_TRY(
        grandpa_justification,
        scale::decode<GrandpaJustification>(encoded_justification.data));

    return outcome::success(std::move(grandpa_justification));
  }

}  // namespace kagome::consensus::grandpa
