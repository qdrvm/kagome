/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/environment_impl.hpp"

#include <boost/optional/optional_io.hpp>
#include <utility>

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/grandpa/authority_manager.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"
#include "consensus/grandpa/justification_observer.hpp"
#include "consensus/grandpa/movable_round_state.hpp"
#include "consensus/grandpa/voting_round_error.hpp"
#include "network/grandpa_transmitter.hpp"
#include "primitives/common.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus::grandpa {

  using primitives::BlockHash;
  using primitives::BlockNumber;
  using primitives::Justification;

  EnvironmentImpl::EnvironmentImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repository,
      std::shared_ptr<AuthorityManager> authority_manager,
      std::shared_ptr<network::GrandpaTransmitter> transmitter,
      std::shared_ptr<parachain::IApprovedAncestor> approved_ancestor,
      LazySPtr<JustificationObserver> justification_observer,
      std::shared_ptr<boost::asio::io_context> main_thread_context)
      : block_tree_{std::move(block_tree)},
        header_repository_{std::move(header_repository)},
        authority_manager_{std::move(authority_manager)},
        transmitter_{std::move(transmitter)},
        approved_ancestor_(std::move(approved_ancestor)),
        justification_observer_(std::move(justification_observer)),
        main_thread_context_{std::move(main_thread_context)},
        logger_{log::createLogger("GrandpaEnvironment", "grandpa")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(header_repository_ != nullptr);
    BOOST_ASSERT(authority_manager_ != nullptr);
    BOOST_ASSERT(transmitter_ != nullptr);

    auto kApprovalLag = "kagome_parachain_approval_checking_finality_lag";
    metrics_registry_->registerGaugeFamily(
        kApprovalLag,
        "How far behind the head of the chain the Approval Checking protocol "
        "wants to vote");
    metric_approval_lag_ = metrics_registry_->registerGaugeMetric(kApprovalLag);

    main_thread_context_.start();
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
    return block_tree_->hasDirectChain(base, block);
  }

  outcome::result<BlockInfo> EnvironmentImpl::bestChainContaining(
      const BlockHash &base, std::optional<VoterSetId> voter_set_id) const {
    SL_DEBUG(logger_, "Finding best chain containing block {}", base);
    OUTCOME_TRY(best_block, block_tree_->getBestContaining(base, std::nullopt));

    // Must finalize block with scheduled/forced change digest first
    auto finalized = block_tree_->getLastFinalized();
    auto block = best_block;
    while (block.number > finalized.number) {
      OUTCOME_TRY(header, header_repository_->getBlockHeader(block.hash));
      if (HasAuthoritySetChange{header}) {
        best_block = block;
      }
      block = {header.number - 1, header.parent_hash};
    }

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

    auto approved = approved_ancestor_->approvedAncestor(finalized, best_block);
    auto lag = best_block.number - approved.number;
    metric_approval_lag_->set(lag);

    SL_DEBUG(logger_, "Found best chain: {}", best_block);
    return std::move(best_block);
  }

  void EnvironmentImpl::onCatchUpRequested(const libp2p::peer::PeerId &_peer_id,
                                           VoterSetId _set_id,
                                           RoundNumber _round_number) {
    REINVOKE_3(main_thread_context_,
               onCatchUpRequested,
               _peer_id,
               _set_id,
               _round_number,
               peer_id,
               set_id,
               round_number);
    network::CatchUpRequest message{.round_number = round_number,
                                    .voter_set_id = set_id};
    transmitter_->sendCatchUpRequest(peer_id, std::move(message));
  }

  void EnvironmentImpl::onCatchUpRespond(
      const libp2p::peer::PeerId &_peer_id,
      VoterSetId _set_id,
      RoundNumber _round_number,
      std::vector<SignedPrevote> _prevote_justification,
      std::vector<SignedPrecommit> _precommit_justification,
      BlockInfo _best_final_candidate) {
    REINVOKE_6(main_thread_context_,
               onCatchUpRespond,
               _peer_id,
               _set_id,
               _round_number,
               _prevote_justification,
               _precommit_justification,
               _best_final_candidate,
               peer_id,
               set_id,
               round_number,
               prevote_justification,
               precommit_justification,
               best_final_candidate);
    SL_DEBUG(logger_, "Send Catch-Up-Response upto round {}", round_number);
    network::CatchUpResponse message{
        .voter_set_id = set_id,
        .round_number = round_number,
        .prevote_justification = std::move(prevote_justification),
        .precommit_justification = std::move(precommit_justification),
        .best_final_candidate = best_final_candidate};
    transmitter_->sendCatchUpResponse(peer_id, std::move(message));
  }

  void EnvironmentImpl::onVoted(RoundNumber round,
                                VoterSetId set_id,
                                const SignedMessage &vote) {
    main_thread_context_.execute([wself{weak_from_this()},
                                  round{std::move(round)},
                                  set_id{std::move(set_id)},
                                  vote]() mutable {
      if (auto self = wself.lock()) {
        SL_VERBOSE(self->logger_,
                "Round #{}: Send {} signed by {} for block {}",
                round,
                visit_in_place(
                    vote.message,
                    [&](const Prevote &) { return "prevote"; },
                    [&](const Precommit &) { return "precommit"; },
                    [&](const PrimaryPropose &) { return "primary propose"; }),
                vote.id,
                vote.getBlockInfo());

        self->transmitter_->sendVoteMessage(
            network::GrandpaVote{{.round_number = std::move(round),
                                  .counter = std::move(set_id),
                                  .vote = std::move(vote)}});
      }
    });
  }

  void EnvironmentImpl::sendState(const libp2p::peer::PeerId &peer_id,
                                  const MovableRoundState &state,
                                  VoterSetId voter_set_id) {
    main_thread_context_.execute([wself{weak_from_this()},
                                  peer_id,
                                  voter_set_id{std::move(voter_set_id)},
                                  state]() mutable {
      if (auto self = wself.lock()) {
        auto send = [&](const SignedMessage &vote) {
          SL_DEBUG(
              self->logger_,
              "Round #{}: Send {} signed by {} for block {} (as send state)",
              state.round_number,
              visit_in_place(
                  vote.message,
                  [&](const Prevote &) { return "prevote"; },
                  [&](const Precommit &) { return "precommit"; },
                  [&](const PrimaryPropose &) { return "primary propose"; }),
              vote.id,
              vote.getBlockInfo());

          ;
          self->transmitter_->sendVoteMessage(
              peer_id,
              network::GrandpaVote{{.round_number = state.round_number,
                                    .counter = voter_set_id,
                                    .vote = vote}});
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
    });
  }

  void EnvironmentImpl::onCommitted(
      RoundNumber _round,
      VoterSetId _voter_ser_id,
      const BlockInfo &_vote,
      const GrandpaJustification &_justification) {
    if (_round == 0) {
      return;
    }

    REINVOKE_4(main_thread_context_,
               onCommitted,
               _round,
               _voter_ser_id,
               _vote,
               _justification,
               round,
               voter_ser_id,
               vote,
               justification);
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
  }

  void EnvironmentImpl::onNeighborMessageSent(RoundNumber _round,
                                              VoterSetId _set_id,
                                              BlockNumber _last_finalized) {
    REINVOKE_3(main_thread_context_,
               onNeighborMessageSent,
               _round,
               _set_id,
               _last_finalized,
               round,
               set_id,
               last_finalized);
    SL_DEBUG(logger_, "Round #{}: Send neighbor message", round);

    network::GrandpaNeighborMessage message{.round_number = round,
                                            .voter_set_id = set_id,
                                            .last_finalized = last_finalized};
    transmitter_->sendNeighborMessage(std::move(message));
  }

  void EnvironmentImpl::applyJustification(
      const BlockInfo &block_info,
      const primitives::Justification &raw_justification,
      ApplyJustificationCb &&cb) {
    auto res = scale::decode<GrandpaJustification>(raw_justification.data);
    if (res.has_error()) {
      cb(res.as_failure());
      return;
    }
    auto &&justification = std::move(res.value());

    if (justification.block_info != block_info) {
      cb(VotingRoundError::JUSTIFICATION_FOR_WRONG_BLOCK);
      return;
    }

    SL_DEBUG(logger_,
             "Trying to apply justification on round #{} for block {}",
             justification.round_number,
             justification.block_info);

    justification_observer_.get()->applyJustification(justification,
                                                      std::move(cb));
  }

  outcome::result<void> EnvironmentImpl::finalize(
      VoterSetId id, const GrandpaJustification &grandpa_justification) {
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
