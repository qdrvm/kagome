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
#include "primitives/common.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus::grandpa {

  using authority::IsBlockFinalized;
  using primitives::BlockHash;
  using primitives::BlockNumber;
  using primitives::Justification;

  EnvironmentImpl::EnvironmentImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repository,
      std::shared_ptr<authority::AuthorityManager> authority_manager,
      std::shared_ptr<network::GrandpaTransmitter> transmitter)
      : block_tree_{std::move(block_tree)},
        header_repository_{std::move(header_repository)},
        authority_manager_{std::move(authority_manager)},
        transmitter_{std::move(transmitter)},
        logger_{log::createLogger("GrandpaEnvironment", "grandpa")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(header_repository_ != nullptr);
    BOOST_ASSERT(authority_manager_ != nullptr);
    BOOST_ASSERT(transmitter_ != nullptr);
  }

  outcome::result<bool> EnvironmentImpl::hasBlock(
      const primitives::BlockHash &block) const {
    return block_tree_->hasBlockHeader(block);
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
      const BlockHash &base,
      std::optional<MembershipCounter> voter_set_id) const {
    SL_DEBUG(logger_, "Finding best chain containing block {}", base);
    OUTCOME_TRY(best_block, block_tree_->getBestContaining(base, std::nullopt));

    // Select best block with actual set_id
    if (voter_set_id.has_value()) {
      while (true) {
        OUTCOME_TRY(header,
                    header_repository_->getBlockHeader(best_block.hash));
        BlockInfo parent_block{header.number - 1, header.parent_hash};

        auto voter_set = authority_manager_->authorities(
            parent_block, IsBlockFinalized{true});

        if (voter_set.has_value()
            && voter_set.value()->id == voter_set_id.value()) {
          // found
          break;
        }

        best_block = parent_block;
      }
    }

    SL_DEBUG(logger_, "Found best chain: {}", best_block);
    return std::move(best_block);
  }

  outcome::result<void> EnvironmentImpl::onCatchUpRequested(
      const libp2p::peer::PeerId &peer_id,
      MembershipCounter set_id,
      RoundNumber round_number) {
    network::CatchUpRequest message{.round_number = round_number,
                                    .voter_set_id = set_id};
    transmitter_->sendCatchUpRequest(peer_id, std::move(message));
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onCatchUpRespond(
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
    transmitter_->sendCatchUpResponse(peer_id, std::move(message));
    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onVoted(RoundNumber round,
                                                 MembershipCounter set_id,
                                                 const SignedMessage &vote) {
    SL_DEBUG(logger_,
             "Round #{}: Send {} signed by {} for block {}",
             round,
             visit_in_place(
                 vote.message,
                 [&](const Prevote &) { return "prevote"; },
                 [&](const Precommit &) { return "precommit"; },
                 [&](const PrimaryPropose &) { return "primary propose"; }),
             vote.id,
             vote.getBlockInfo());

    network::GrandpaVote message{
        {.round_number = round, .counter = set_id, .vote = vote}};
    transmitter_->sendVoteMessage(std::move(message));
    return outcome::success();
  }

  void EnvironmentImpl::sendState(const libp2p::peer::PeerId &peer_id,
                                  const MovableRoundState &state,
                                  MembershipCounter voter_set_id) {
    auto send = [&](const SignedMessage &vote) {
      SL_DEBUG(logger_,
               "Round #{}: Send {} signed by {} for block {} (as send state)",
               state.round_number,
               visit_in_place(
                   vote.message,
                   [&](const Prevote &) { return "prevote"; },
                   [&](const Precommit &) { return "precommit"; },
                   [&](const PrimaryPropose &) { return "primary propose"; }),
               vote.id,
               vote.getBlockInfo());

      network::GrandpaVote message{{.round_number = state.round_number,
                                    .counter = voter_set_id,
                                    .vote = vote}};
      transmitter_->sendVoteMessage(peer_id, std::move(message));
    };

    for (const auto &vv : state.votes) {
      visit_in_place(
          vv,
          [&](const SignedMessage &vote) { send(vote); },
          [&](const EquivocatorySignedMessage &pair_vote) {
            send(pair_vote.first);
            send(pair_vote.second);
          });
    }
  }

  outcome::result<void> EnvironmentImpl::onCommitted(
      RoundNumber round,
      MembershipCounter voter_ser_id,
      const BlockInfo &vote,
      const GrandpaJustification &justification) {
    if (round == 0) {
      return outcome::success();
    }

    SL_DEBUG(logger_, "Round #{}: Send commit of block {}", round, vote);

    network::FullCommitMessage message{
        .round = round,
        .set_id = voter_ser_id,
        .message = {.target_hash = vote.hash, .target_number = vote.number}};
    for (const auto &item : justification.items) {
      BOOST_ASSERT(item.is<Precommit>());
      const auto &precommit = boost::relaxed_get<Precommit>(item.message);
      message.message.precommits.push_back(precommit);
      message.message.auth_data.emplace_back(item.signature, item.id);
    }
    transmitter_->sendCommitMessage(std::move(message));

    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::onNeighborMessageSent(
      RoundNumber round, MembershipCounter set_id, BlockNumber last_finalized) {
    SL_DEBUG(logger_, "Round #{}: Send neighbor message", round);

    network::GrandpaNeighborMessage message{.round_number = round,
                                            .voter_set_id = set_id,
                                            .last_finalized = last_finalized};
    transmitter_->sendNeighborMessage(std::move(message));

    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::applyJustification(
      const BlockInfo &block_info,
      const primitives::Justification &raw_justification) {
    auto justification_observer = justification_observer_.lock();
    BOOST_ASSERT(justification_observer);

    OUTCOME_TRY(
        justification,
        scale::decode<grandpa::GrandpaJustification>(raw_justification.data));

    SL_DEBUG(logger_,
             "Trying to apply justification on round #{} for block {}",
             justification.round_number,
             justification.block_info);

    OUTCOME_TRY(
        justification_observer->applyJustification(block_info, justification));

    return outcome::success();
  }

  outcome::result<void> EnvironmentImpl::finalize(
      MembershipCounter id, const GrandpaJustification &grandpa_justification) {
    primitives::Justification justification;
    OUTCOME_TRY(enc, scale::encode(grandpa_justification));
    justification.data.put(enc);
    OUTCOME_TRY(block_tree_->finalize(grandpa_justification.block_info.hash,
                                      justification));

    return outcome::success();
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
