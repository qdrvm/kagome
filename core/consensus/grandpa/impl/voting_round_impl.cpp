/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/voting_round_impl.hpp"

#include <unordered_set>

#include "blockchain/block_tree_error.hpp"
#include "consensus/grandpa/ancestry_verifier.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/grandpa.hpp"
#include "consensus/grandpa/grandpa_config.hpp"
#include "consensus/grandpa/grandpa_context.hpp"
#include "consensus/grandpa/historical_votes.hpp"
#include "consensus/grandpa/vote_crypto_provider.hpp"
#include "consensus/grandpa/vote_graph.hpp"
#include "consensus/grandpa/vote_tracker.hpp"
#include "consensus/grandpa/vote_types.hpp"
#include "consensus/grandpa/vote_weight.hpp"
#include "consensus/grandpa/voting_round_error.hpp"
#include "consensus/grandpa/voting_round_update.hpp"

namespace kagome::consensus::grandpa {

  namespace {
    template <typename T>
    auto convertToPrimaryPropose(const T &vote) {
      return PrimaryPropose(vote.number, vote.hash);
    }

    template <typename T>
    auto convertToPrevote(const T &vote) {
      return Prevote(vote.number, vote.hash);
    }

    template <typename T>
    auto convertToPrecommit(const T &vote) {
      return Precommit(vote.number, vote.hash);
    }

    template <typename T>
    auto convertToBlockInfo(const T &vote) {
      return BlockInfo(vote.number, vote.hash);
    }

    template <typename D>
    auto toMilliseconds(const D &duration) {
      return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    }
  }  // namespace

  VotingRoundImpl::VotingRoundImpl(
      const std::shared_ptr<Grandpa> &grandpa,
      const GrandpaConfig &config,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<Environment> env,
      std::shared_ptr<VoteCryptoProvider> vote_crypto_provider,
      std::shared_ptr<VoteTracker> prevotes,
      std::shared_ptr<VoteTracker> precommits,
      std::shared_ptr<VoteGraph> vote_graph,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : voter_set_{config.voters},
        round_number_{config.round_number},
        duration_{config.duration},
        id_{config.id},
        grandpa_(grandpa),
        hasher_{std::move(hasher)},
        env_{std::move(env)},
        vote_crypto_provider_{std::move(vote_crypto_provider)},
        graph_{std::move(vote_graph)},
        scheduler_{std::move(scheduler)},
        prevotes_{std::move(prevotes)},
        precommits_{std::move(precommits)} {
    BOOST_ASSERT(not grandpa_.expired());
    BOOST_ASSERT(vote_crypto_provider_ != nullptr);
    BOOST_ASSERT(prevotes_ != nullptr);
    BOOST_ASSERT(precommits_ != nullptr);
    BOOST_ASSERT(env_ != nullptr);
    BOOST_ASSERT(graph_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);

    // calculate super-majority
    auto faulty = (voter_set_->totalWeight() - 1) / 3;
    threshold_ = voter_set_->totalWeight() - faulty;

    // Check if node is primary
    auto index = round_number_ % voter_set_->size();
    isPrimary_ = voter_set_->voterId(index) == outcome::success(id_);

    prevote_equivocators_.resize(voter_set_->size(), false);
    precommit_equivocators_.resize(voter_set_->size(), false);

    SL_DEBUG(logger_,
             "Round #{}: Created with voter set #{}",
             round_number_,
             voter_set_->id());
  }

  VotingRoundImpl::VotingRoundImpl(
      const std::shared_ptr<Grandpa> &grandpa,
      const GrandpaConfig &config,
      std::shared_ptr<crypto::Hasher> hasher,
      const std::shared_ptr<Environment> &env,
      const std::shared_ptr<VoteCryptoProvider> &vote_crypto_provider,
      const std::shared_ptr<VoteTracker> &prevotes,
      const std::shared_ptr<VoteTracker> &precommits,
      const std::shared_ptr<VoteGraph> &vote_graph,
      const std::shared_ptr<libp2p::basic::Scheduler> &scheduler,
      const std::shared_ptr<VotingRound> &previous_round)
      : VotingRoundImpl(grandpa,
                        config,
                        std::move(hasher),
                        env,
                        vote_crypto_provider,
                        prevotes,
                        precommits,
                        vote_graph,
                        scheduler) {
    BOOST_ASSERT(previous_round != nullptr);

    previous_round_ = previous_round;
    last_finalized_block_ = previous_round->finalizedBlock().value_or(
        previous_round->lastFinalizedBlock());
  }

  VotingRoundImpl::VotingRoundImpl(
      const std::shared_ptr<Grandpa> &grandpa,
      const GrandpaConfig &config,
      std::shared_ptr<crypto::Hasher> hasher,
      const std::shared_ptr<Environment> &env,
      const std::shared_ptr<VoteCryptoProvider> &vote_crypto_provider,
      const std::shared_ptr<VoteTracker> &prevotes,
      const std::shared_ptr<VoteTracker> &precommits,
      const std::shared_ptr<VoteGraph> &vote_graph,
      const std::shared_ptr<libp2p::basic::Scheduler> &scheduler,
      const MovableRoundState &round_state)
      : VotingRoundImpl(grandpa,
                        config,
                        std::move(hasher),
                        env,
                        vote_crypto_provider,
                        prevotes,
                        precommits,
                        vote_graph,
                        scheduler) {
    last_finalized_block_ = round_state.last_finalized_block;

    if (round_number_ != 0) {
      VotingRoundUpdate update{.round = *this};
      for (auto &vote : round_state.votes) {
        update.vote(vote);
      }
      update.update();

      // Round might be no finalized, if provided state has not enough prevotes
      // (i.e. state was made by justification in commit). In this case we have
      // fallback way to finalize than basing on supermajority of precommits. It
      // is enough to be finalized, but no completable.
      if (not finalized_.has_value()) {
        if (precommits_->getTotalWeight() >= threshold_) {
          auto possible_to_finalize = [&](const VoteWeight &weight) {
            return weight.total(VoteType::Precommit,
                                precommit_equivocators_,
                                *voter_set_)
                >= threshold_;
          };

          finalized_ = graph_->findAncestor(VoteType::Precommit,
                                            last_finalized_block_,
                                            std::move(possible_to_finalize));

          BOOST_ASSERT(finalized_.has_value());
        }
      }

    } else {
      // Zero-round is always self-finalized
      finalized_ = last_finalized_block_;
      completable_ = true;
    }
  }

  bool VotingRoundImpl::hasKeypair() const {
    return id_.has_value();
  }

  void VotingRoundImpl::play() {
    if (stage_ != Stage::INIT) {
      return;
    }

    stage_ = Stage::START;

    SL_DEBUG(logger_, "Round #{}: Start round", round_number_);

    pending_timer_handle_ = scheduler_->scheduleWithHandle(
        [wself{weak_from_this()}] {
          if (auto self = wself.lock()) {
            self->pending();
          }
        },
        pending_interval_);

    sendNeighborMessage();

    // Current local time (Tstart)
    start_time_ = scheduler_->now();

    // Derive-Primary
    // see ctor

    if (isPrimary_ and previous_round_) {
      SL_DEBUG(logger_, "Node is primary proposer at round #{}", round_number_);

      // Broadcast Commit-message with previous round best final candidate
      //  (or last finalized otherwise)
      // spec: Broadcast(M vr ¡1;Fin (Best-Final-Candidate(r-1)))
      previous_round_->doCommit();

      // if Best-Final-Candidate greater than Last-Finalized-Block
      // spec: if Best-Final-Candidate(r ¡ 1) > Last-Finalized-Block
      if (previous_round_->bestFinalCandidate().number
          > last_finalized_block_.number) {
        doProposal();
      }
    }

    startPrevoteStage();
  }

  void VotingRoundImpl::startPrevoteStage() {
    if (stage_ == Stage::COMPLETED) {
      return;
    }
    BOOST_ASSERT(stage_ == Stage::START);

    stage_ = Stage::START_PREVOTE;

    SL_DEBUG(logger_, "Round #{}: Start prevote stage", round_number_);

    // Continue to receive messages
    // until T>=Tstart + 2 * Duration or round is completable
    // spec: Receive-Messages(until Time>=Tr+2T or r is completable)

    if (completable()) {
      SL_DEBUG(logger_, "Round #{} is already completable", round_number_);
      stage_ = Stage::PREVOTE_RUNS;
      endPrevoteStage();
      return;
    }

    stage_timer_handle_ = scheduler_->scheduleWithHandle(
        [wself{weak_from_this()}] {
          if (auto self = wself.lock()) {
            if (self->stage_ == Stage::PREVOTE_RUNS) {
              SL_DEBUG(self->logger_,
                       "Round #{}: Time of prevote stage is out",
                       self->round_number_);
              self->endPrevoteStage();
            }
          }
        },
        toMilliseconds(duration_ * 2 - (scheduler_->now() - start_time_)));

    on_complete_handler_ = [this] {
      if (stage_ == Stage::PREVOTE_RUNS) {
        SL_DEBUG(logger_, "Round #{}: Became completable", round_number_);
        endPrevoteStage();
      }
    };

    stage_ = Stage::PREVOTE_RUNS;
  }

  void VotingRoundImpl::endPrevoteStage() {
    if (stage_ == Stage::COMPLETED) {
      return;
    }
    BOOST_ASSERT(stage_ == Stage::PREVOTE_RUNS);

    stage_timer_handle_.reset();
    on_complete_handler_ = nullptr;

    stage_ = Stage::END_PREVOTE;

    SL_DEBUG(logger_, "Round #{}: End prevote stage", round_number_);

    // Broadcast vote for prevote stage
    doPrevote();

    startPrecommitStage();
  }

  void VotingRoundImpl::startPrecommitStage() {
    if (stage_ == Stage::COMPLETED) {
      return;
    }
    BOOST_ASSERT(stage_ == Stage::END_PREVOTE);

    stage_ = Stage::START_PRECOMMIT;

    SL_DEBUG(logger_, "Round #{}: Start precommit stage", round_number_);

    // Continue to receive messages
    // until T>=Tstart + 4 * Duration or round is completable

    // spec: Receive-Messages(
    //  until Bpv>=Best-Final-Candidate(r-1)
    //  and (Time>=Tr+4T or r is completable)
    // )

    if (completable()) {
      SL_DEBUG(logger_, "Round #{} is already completable", round_number_);
      stage_ = Stage::PRECOMMIT_RUNS;
      endPrecommitStage();
      return;
    }

    stage_timer_handle_ = scheduler_->scheduleWithHandle(
        [wself{weak_from_this()}] {
          if (auto self = wself.lock()) {
            if (self->stage_ == Stage::PRECOMMIT_RUNS) {
              SL_DEBUG(self->logger_,
                       "Round #{}: Time of precommit stage is out",
                       self->round_number_);
              self->endPrecommitStage();
            }
          }
        },
        toMilliseconds(duration_ * 4 - (scheduler_->now() - start_time_)));

    on_complete_handler_ = [this] {
      if (stage_ == Stage::PRECOMMIT_RUNS) {
        SL_DEBUG(logger_, "Round #{}: Became completable", round_number_);
        endPrecommitStage();
      }
    };

    stage_ = Stage::PRECOMMIT_RUNS;
  }

  void VotingRoundImpl::endPrecommitStage() {
    if (stage_ == Stage::COMPLETED) {
      return;
    }
    BOOST_ASSERT(stage_ == Stage::PRECOMMIT_RUNS
                 || stage_ == Stage::PRECOMMIT_WAITS_FOR_PREVOTES);

    stage_timer_handle_.reset();

    // https://github.com/paritytech/finality-grandpa/blob/8c45a664c05657f0c71057158d3ba555ba7d20de/src/voter/voting_round.rs#L630-L633
    if (not prevote_ghost_) {
      stage_ = Stage::PRECOMMIT_WAITS_FOR_PREVOTES;
      logger_->debug("Round #{}: Precommit waits for prevotes", round_number_);
      return;
    }

    on_complete_handler_ = nullptr;

    stage_ = Stage::END_PRECOMMIT;

    SL_DEBUG(logger_, "Round #{}: End precommit stage", round_number_);

    // Broadcast vote for precommit stage
    doPrecommit();

    startWaitingStage();
  }

  void VotingRoundImpl::startWaitingStage() {
    if (stage_ == Stage::COMPLETED) {
      return;
    }
    BOOST_ASSERT(stage_ == Stage::END_PRECOMMIT);

    stage_ = Stage::START_WAITING;

    // Continue to receive messages until current round is completable and
    // previous one is finalizable and last finalized better than the best final
    // candidate of previous round

    // spec: Receive-Messages(
    //    until r is completable
    //    and Finalizable(r-1)
    //    and Last-Finalized-Block>Best-Final-Candidate(r-1)
    // )

    const bool is_ready_to_end =
        finalized_.has_value()
        and finalized_->number >= prevBestFinalCandidate().number;

    if (is_ready_to_end) {
      SL_DEBUG(logger_,
               "Round #{}: Conditions for final stage are satisfied already",
               round_number_);
      stage_ = Stage::WAITING_RUNS;
      endWaitingStage();
      return;
    }

    SL_DEBUG(logger_, "Round #{}: Start final stage", round_number_);

    on_complete_handler_ = [&] {
      const bool is_ready_to_end =
          finalized_.has_value()
          and finalized_->number >= prevBestFinalCandidate().number;

      if (is_ready_to_end) {
        SL_DEBUG(logger_,
                 "Round #{}: Conditions for final stage are met",
                 round_number_);
        endWaitingStage();
      }
    };

    stage_ = Stage::WAITING_RUNS;
  }

  void VotingRoundImpl::endWaitingStage() {
    if (stage_ == Stage::COMPLETED) {
      return;
    }
    BOOST_ASSERT(stage_ == Stage::WAITING_RUNS);

    stage_timer_handle_.reset();
    on_complete_handler_ = nullptr;

    // Final attempt to finalize round what should be success
    BOOST_ASSERT(finalized_.has_value());
    attemptToFinalizeRound();

    end();
  }

  void VotingRoundImpl::end() {
    if (stage_ != Stage::COMPLETED) {
      SL_DEBUG(logger_, "Round #{}: End round", round_number_);
      on_complete_handler_ = nullptr;
      stage_timer_handle_.reset();
      pending_timer_handle_.reset();
      stage_ = Stage::COMPLETED;
    }
  }

  void VotingRoundImpl::doProposal() {
    // Doing primary proposal is no longer actual
    if (not previous_round_) {
      return;
    }

    // Don't change early defined primary vote
    if (primary_vote_.has_value()) {
      sendProposal(convertToPrimaryPropose(primary_vote_.value()));
      return;
    }

    // Send primary propose
    // @spec Broadcast(M vr ¡1;Prim (Best-Final-Candidate(r-1)))

    BOOST_ASSERT_MSG(not primary_vote_.has_value(),
                     "Primary proposal must be once for a round");

    primary_vote_ = previous_round_->bestFinalCandidate();

    sendProposal(convertToPrimaryPropose(primary_vote_.value()));
  }

  void VotingRoundImpl::sendProposal(const PrimaryPropose &primary_proposal) {
    SL_DEBUG(logger_,
             "Round #{}: Sending primary proposal of block {}",
             round_number_,
             primary_proposal);

    auto signed_primary_proposal_opt =
        vote_crypto_provider_->signPrimaryPropose(primary_proposal);

    if (not signed_primary_proposal_opt.has_value()) {
      logger_->error(
          "Round #{}: Primary proposal was not sent: Can't sign message",
          round_number_);
      return;
    }
    const auto &signed_primary_proposal = signed_primary_proposal_opt.value();
    onProposal({}, signed_primary_proposal, Propagation::NEEDLESS);
    env_->onVoted(round_number_, voter_set_->id(), signed_primary_proposal);
  }

  void VotingRoundImpl::doPrevote() {
    // Don't change defined vote to avoid equivocation
    if (prevote_.has_value()) {
      sendPrevote(convertToPrevote(prevote_.value()));
      return;
    }

    // spec: L <- Best-Final-Candidate(r-1)
    const auto best_final_candidate = prevBestFinalCandidate();

    // spec: Bpv <- GRANDPA-GHOST(r)
    const auto best_chain =
        env_->bestChainContaining(best_final_candidate.hash, voter_set_->id());
    const auto best_prevote_candidate =
        best_chain.has_value() ? convertToBlockInfo(best_chain.value())
                               : last_finalized_block_;

    // spec: N <- Bpv
    prevote_ = best_prevote_candidate;

    // spec: if Received(Bprim) and Bpv >= Bprim > L
    if (primary_vote_.has_value()) {
      const auto &primary = primary_vote_.value();

      if (best_prevote_candidate.number >= primary.number
          and primary.number > best_final_candidate.number) {
        // spec: N <- Bprim
        prevote_ = primary;
      }
    }

    // Broadcast vote for prevote stage
    // spec: Broadcast(Bpv)
    sendPrevote(convertToPrevote(prevote_.value()));
  }

  void VotingRoundImpl::sendPrevote(const Prevote &prevote) {
    SL_DEBUG(logger_,
             "Round #{}: Sending prevote for block {}",
             round_number_,
             prevote);

    auto signed_prevote_opt = vote_crypto_provider_->signPrevote(prevote);
    if (not signed_prevote_opt.has_value()) {
      logger_->error("Round #{}: Prevote was not sent: Can't sign message",
                     round_number_);
      return;
    }
    auto &signed_prevote = signed_prevote_opt.value();
    if (onPrevote({}, signed_prevote, Propagation::NEEDLESS)) {
      update(IsPreviousRoundChanged{false},
             IsPrevotesChanged{true},
             IsPrecommitsChanged{false});
    }
    env_->onVoted(round_number_, voter_set_->id(), signed_prevote);
  }

  void VotingRoundImpl::doPrecommit() {
    // Don't change defined vote to avoid equivocation
    if (precommit_.has_value()) {
      sendPrecommit(convertToPrecommit(precommit_.value()));
      return;
    }

    // we wait for the last round's estimate to be equal to or
    // the ancestor of the current round's p-Ghost before precommitting.

    BOOST_ASSERT(prevote_ghost_.has_value());
    const auto &prevote_ghost = prevote_ghost_.value();

    auto last_round_estimate = prevBestFinalCandidate();

    // We should precommit if current state contains prevote, and it is
    // either equal to the last round estimate or is descendant of it
    const bool should_precommit =
        prevote_ghost == last_round_estimate
        or env_->hasAncestry(last_round_estimate.hash, prevote_ghost.hash);

    if (should_precommit) {
      precommit_ = prevote_ghost;

      sendPrecommit(convertToPrecommit(precommit_.value()));
      return;
    }
  }

  void VotingRoundImpl::sendPrecommit(const Precommit &precommit) {
    SL_DEBUG(logger_,
             "Round #{}: Sending precommit for block {}",
             round_number_,
             precommit);

    auto signed_precommit_opt = vote_crypto_provider_->signPrecommit(precommit);
    if (not signed_precommit_opt.has_value()) {
      logger_->error("Round #{}: Precommit was not sent: Can't sign message",
                     round_number_);
      return;
    }
    auto &signed_precommit = signed_precommit_opt.value();
    if (onPrecommit({}, signed_precommit, Propagation::NEEDLESS)) {
      update(IsPreviousRoundChanged{false},
             IsPrevotesChanged{false},
             IsPrecommitsChanged{true});
    }
    env_->onVoted(round_number_, voter_set_->id(), signed_precommit);
  }

  void VotingRoundImpl::doFinalize() {
    BOOST_ASSERT(finalized_.has_value());

    const auto &block = finalized_.value();

    SL_DEBUG(
        logger_, "Round #{}: Finalizing on block {}", round_number_, block);

    GrandpaJustification justification{
        .round_number = round_number_,
        .block_info = block,
        .items = getPrecommitJustification(block, precommits_->getMessages())};

    auto res = env_->finalize(voter_set_->id(), justification);
    if (res.has_error()) {
      SL_WARN(logger_,
              "Round #{}: Finalizing on block {} is failed: {}",
              round_number_,
              block,
              res.error());
    }
  }

  void VotingRoundImpl::doCommit() {
    if (not finalized_.has_value()) {
      return;
    }

    auto block = finalized_.value();
    GrandpaJustification justification{
        .round_number = round_number_,
        .block_info = block,
        .items = getPrecommitJustification(block, precommits_->getMessages())};

    if (auto r = env_->makeAncestry(justification); not r) {
      SL_ERROR(logger_, "doCommit: makeAncestry: {}", r.error());
    }

    SL_DEBUG(logger_,
             "Round #{}: Sending commit message for block {}",
             round_number_,
             block);

    std::ignore = applyJustification(justification);
    env_->onCommitted(round_number_, voter_set_->id(), block, justification);
  }

  bool VotingRoundImpl::isPrimary(const Id &id) const {
    auto index = round_number_ % voter_set_->size();
    return voter_set_->voterId(index) == outcome::success(id);
  }

  outcome::result<void> VotingRoundImpl::applyJustification(
      const GrandpaJustification &justification) {
    // validate message
    OUTCOME_TRY(validatePrecommitJustification(justification));

    SL_DEBUG(logger_,
             "Round #{}: Finalisation of round is received for block {}",
             round_number_,
             justification.block_info);

    VotingRoundUpdate update{.round = *this};
    for (auto &vote : justification.items) {
      update.vote(vote);
    }
    update.update();

    if (not finalized_.has_value()) {
      if (precommits_->getTotalWeight() >= threshold_) {
        auto possible_to_finalize = [&](const VoteWeight &weight) {
          return weight.total(
                     VoteType::Precommit, precommit_equivocators_, *voter_set_)
              >= threshold_;
        };

        finalized_ = graph_->findAncestor(VoteType::Precommit,
                                          justification.block_info,
                                          std::move(possible_to_finalize));

        BOOST_ASSERT(finalized_.has_value());
      } else {
        return VotingRoundError::ROUND_IS_NOT_FINALIZABLE;
      }
    }

    if (not env_->hasAncestry(justification.block_info.hash,
                              finalized_.value().hash)) {
      return VotingRoundError::
          JUSTIFIED_BLOCK_IS_GREATER_THAN_ACTUALLY_FINALIZED;
    }

    OUTCOME_TRY(env_->finalize(voter_set_->id(), justification));
    return outcome::success();
  }

  outcome::result<void> VotingRoundImpl::validatePrecommitJustification(
      const GrandpaJustification &justification) const {
    AncestryVerifier ancestry_verifier(justification.votes_ancestries,
                                       *hasher_);
    auto has_ancestry = [&](const BlockInfo &ancestor,
                            const BlockInfo &descendant) {
      return ancestry_verifier.hasAncestry(ancestor, descendant)
          or env_->hasAncestry(ancestor.hash, descendant.hash);
    };

    size_t total_weight = 0;

    auto threshold = threshold_;
    std::unordered_map<Id, BlockInfo> validators;
    std::unordered_set<Id> equivocators;

    for (const auto &signed_precommit : justification.items) {
      // Skip known equivocators
      if (auto index = voter_set_->voterIndex(signed_precommit.id);
          index.has_value()) {
        if (precommit_equivocators_.at(index.value())) {
          continue;
        }
      }

      // Verify signatures
      if (not vote_crypto_provider_->verifyPrecommit(signed_precommit)) {
        SL_WARN(
            logger_,
            "Round #{}: Precommit signed by {} was rejected: invalid signature",
            round_number_,
            signed_precommit.id);
        return VotingRoundError::INVALID_SIGNATURE;
      }

      // check that every signed precommit corresponds to the vote (i.e.
      // signed_precommits are descendants of the vote). If so add weight of
      // that voter to the total weight

      if (auto [it, success] = validators.emplace(
              signed_precommit.id, signed_precommit.getBlockInfo());
          success) {
        // New vote
        auto weight_opt = voter_set_->voterWeight(signed_precommit.id);
        if (!weight_opt) {
          SL_DEBUG(logger_,
                   "Voter {} is not in the current voter set",
                   signed_precommit.id.toHex());
          continue;
        }
        if (has_ancestry(justification.block_info,
                         signed_precommit.getBlockInfo())) {
          total_weight += weight_opt.value();
        } else {
          SL_DEBUG(logger_,
                   "Vote does not have ancestry with target block: "
                   "vote={} target={}",
                   justification.block_info,
                   signed_precommit.getBlockInfo());
        }

      } else if (equivocators.emplace(signed_precommit.id).second) {
        // Detected equivocation
        if (has_ancestry(justification.block_info, it->second)) {
          auto weight = voter_set_->voterWeight(signed_precommit.id).value();
          total_weight -= weight;
          threshold -= weight;
        } else {
          SL_DEBUG(logger_,
                   "Vote does not have ancestry with target block: "
                   "vote={} target={}",
                   justification.block_info,
                   signed_precommit.getBlockInfo());
        }

      } else {
        // Detected duplicate of equivotation
        SL_WARN(
            logger_,
            "Round #{}: Received third precommit of caught equivocator from {}",
            round_number_,
            signed_precommit.id);
        return VotingRoundError::REDUNDANT_EQUIVOCATION;
      }
    }

    if (total_weight < threshold) {
      SL_WARN(logger_,
              "Round #{}: Received justification does not have super-majority: "
              "total_weight={} < threshold={}",
              round_number_,
              total_weight,
              threshold);
      return VotingRoundError::NOT_ENOUGH_WEIGHT;
    }

    return outcome::success();
  }

  void VotingRoundImpl::attemptToFinalizeRound() {
    if (stage_ == Stage::COMPLETED) {
      return;
    }

    if (finalized_.has_value()) {
      doFinalize();
      if (on_complete_handler_) {
        on_complete_handler_();
      }
      return;
    }

    if (not completable_) {
      SL_DEBUG(logger_,
               "Round #{}: Round not finalized yet: not completable",
               round_number_);
    } else if (not finalized_.has_value()) {
      SL_DEBUG(logger_,
               "Round #{}: Round not finalized yet: not finalizable",
               round_number_);
    } else {
      SL_DEBUG(logger_,
               "Round #{}: Round not finalized yet: unknown reason",
               round_number_);
    }
  }

  RoundNumber VotingRoundImpl::roundNumber() const {
    return round_number_;
  }

  VoterSetId VotingRoundImpl::voterSetId() const {
    return voter_set_->id();
  }

  void VotingRoundImpl::onProposal(OptRef<GrandpaContext> grandpa_context,
                                   const SignedMessage &proposal,
                                   Propagation propagation) {
    if (not isPrimary(proposal.id)) {
      logger_->warn(
          "Round #{}: Proposal signed by {} was rejected: "
          "voter is not primary",
          round_number_,
          proposal.id);
      return;
    }

    if (grandpa_context) {
      grandpa_context->checked_signature_counter++;
    }

    bool isValid = vote_crypto_provider_->verifyPrimaryPropose(proposal);
    if (not isValid) {
      logger_->warn(
          "Round #{}: Proposal signed by {} was rejected: "
          "invalid signature",
          round_number_,
          proposal.id);

      if (grandpa_context) {
        grandpa_context->invalid_signature_counter++;
      }

      return;
    }

    auto result = voter_set_->indexAndWeight(proposal.id);
    if (!result) {
      if (grandpa_context) {
        grandpa_context->unknown_voter_counter++;
      }
    }

    SL_DEBUG(logger_,
             "Round #{}: Proposal signed by {} was accepted for block {}",
             round_number_,
             proposal.id,
             proposal.getBlockInfo());

    if (primary_vote_.has_value()) {
      propagation = Propagation::NEEDLESS;
    } else {
      // Check if node hasn't block
      if (not env_->hasBlock(proposal.getBlockHash())) {
        if (grandpa_context) {
          grandpa_context->missing_blocks.emplace(proposal.getBlockInfo());
        }
        return;
      }
    }

    primary_vote_.emplace(proposal.getBlockInfo());

    if (propagation == Propagation::REQUESTED) {
      env_->onVoted(round_number_, voter_set_->id(), proposal);
    }
  }

  bool VotingRoundImpl::onPrevote(OptRef<GrandpaContext> grandpa_context,
                                  const SignedMessage &prevote,
                                  Propagation propagation) {
    if (grandpa_context) {
      grandpa_context->checked_signature_counter++;
    }

    bool isValid = vote_crypto_provider_->verifyPrevote(prevote);
    if (not isValid) {
      logger_->warn(
          "Round #{}: Prevote signed by {} was rejected: invalid signature",
          round_number_,
          prevote.id);

      if (grandpa_context) {
        grandpa_context->invalid_signature_counter++;
      }

      return false;
    }

    if (auto result = onSigned<Prevote>(grandpa_context, prevote);
        result.has_failure()) {
      if (result == outcome::failure(VotingRoundError::DUPLICATED_VOTE)) {
        return false;
      }
      if (result
          == outcome::failure(VotingRoundError::VOTE_OF_KNOWN_EQUIVOCATOR)) {
        return false;
      }
      if (result == outcome::failure(VotingRoundError::UNKNOWN_VOTER)) {
        if (grandpa_context) {
          grandpa_context->unknown_voter_counter++;
        }
      }
      if (result != outcome::failure(VotingRoundError::EQUIVOCATED_VOTE)) {
        logger_->warn("Round #{}: Prevote signed by {} was rejected: {}",
                      round_number_,
                      prevote.id,
                      result.error());
        return false;
      }
    }

    SL_DEBUG(logger_,
             "Round #{}: Prevote signed by {} was accepted for block {}",
             round_number_,
             prevote.id,
             prevote.getBlockInfo());

    if (id_ == prevote.id) {
      if (not prevote_.has_value()) {
        prevote_.emplace(prevote.getBlockInfo());
        SL_DEBUG(logger_, "Round #{}: Own prevote was restored", round_number_);
      }
      propagation = Propagation::NEEDLESS;
    }

    if (propagation == Propagation::REQUESTED) {
      env_->onVoted(round_number_, voter_set_->id(), prevote);
    }

    // precommit if we were still waiting for prevotes and now have enough
    // prevotes to construct precommit, or previous prevote ghost was updated
    // https://github.com/paritytech/finality-grandpa/blob/8c45a664c05657f0c71057158d3ba555ba7d20de/src/voter/voting_round.rs#L630
    if (stage_ == Stage::PRECOMMIT_WAITS_FOR_PREVOTES) {
      if (updateGrandpaGhost()) {
        endPrecommitStage();
      }
    }

    return true;
  }

  bool VotingRoundImpl::onPrecommit(OptRef<GrandpaContext> grandpa_context,
                                    const SignedMessage &precommit,
                                    Propagation propagation) {
    if (grandpa_context) {
      grandpa_context->checked_signature_counter++;
    }

    bool isValid = vote_crypto_provider_->verifyPrecommit(precommit);
    if (not isValid) {
      logger_->warn(
          "Round #{}: Precommit signed by {} was rejected: "
          "invalid signature",
          round_number_,
          precommit.id);

      if (grandpa_context) {
        grandpa_context->invalid_signature_counter++;
      }

      return false;
    }

    if (auto result = onSigned<Precommit>(grandpa_context, precommit);
        result.has_failure()) {
      if (result == outcome::failure(VotingRoundError::DUPLICATED_VOTE)) {
        return false;
      }
      if (result
          == outcome::failure(VotingRoundError::VOTE_OF_KNOWN_EQUIVOCATOR)) {
        return false;
      }
      if (result != outcome::failure(VotingRoundError::EQUIVOCATED_VOTE)) {
        logger_->warn("Round #{}: Precommit signed by {} was rejected: {}",
                      round_number_,
                      precommit.id,
                      result.error());
        return false;
      }
    }

    SL_DEBUG(logger_,
             "Round #{}: Precommit signed by {} was accepted for block {}",
             round_number_,
             precommit.id,
             precommit.getBlockInfo());

    if (id_ == precommit.id) {
      if (not precommit_.has_value()) {
        precommit_.emplace(precommit.getBlockInfo());
        SL_DEBUG(
            logger_, "Round #{}: Own precommit was restored", round_number_);
      }
      propagation = Propagation::NEEDLESS;
    }

    if (propagation == Propagation::REQUESTED) {
      env_->onVoted(round_number_, voter_set_->id(), precommit);
    }

    return true;
  }

  void VotingRoundImpl::update(IsPreviousRoundChanged is_previous_round_changed,
                               IsPrevotesChanged is_prevotes_changed,
                               IsPrecommitsChanged is_precommits_changed) {
    bool need_to_update_grandpa_ghost =
        is_previous_round_changed or is_prevotes_changed;

    bool need_to_update_estimate =
        is_precommits_changed or need_to_update_grandpa_ghost;

    if (need_to_update_grandpa_ghost) {
      if (updateGrandpaGhost()) {
        need_to_update_estimate = true;
      }
      if (prevote_ghost_) {
        scheduler_->schedule([wself{weak_from_this()}] {
          if (auto self = wself.lock()) {
            if (self->stage_ == Stage::PRECOMMIT_WAITS_FOR_PREVOTES) {
              self->endPrecommitStage();
            }
          }
        });
      }
    }

    if (need_to_update_estimate) {
      if (updateEstimate()) {
        attemptToFinalizeRound();

        if (auto grandpa = grandpa_.lock()) {
          grandpa->updateNextRound(round_number_);
        }
      }
    }

    bool can_start_next_round;  // NOLINT(cppcoreguidelines-init-variables)

    // Start next round only when previous round estimate is finalized
    if (previous_round_) {
      // Either it was already finalized in the previous round or it must be
      // finalized in the current round
      can_start_next_round = previous_round_->finalizedBlock().has_value();
    } else {
      // When we catch up to a round we complete the round without any last
      // round state. in this case we already started a new round after we
      // caught up so this guard is unneeded.
      can_start_next_round = true;
    }

    // Start next round only when current round is completable
    can_start_next_round = can_start_next_round and completable_;

    // Play new round
    // spec: Play-Grandpa-round(r + 1);

    if (can_start_next_round) {
      scheduler_->schedule(
          [grandpa_wp = std::move(grandpa_), round_wp{weak_from_this()}] {
            if (auto grandpa = grandpa_wp.lock()) {
              if (auto round = round_wp.lock()) {
                grandpa->tryExecuteNextRound(round);
              }
            }
          });
    }
  }

  template <typename T>
  outcome::result<void> VotingRoundImpl::onSigned(
      OptRef<GrandpaContext> grandpa_context, const SignedMessage &vote) {
    auto save_historical_vote = [&] {
      if (auto grandpa = grandpa_.lock()) {
        grandpa->saveHistoricalVote(
            voter_set_->id(), round_number_, vote, vote.id == id_);
      }
    };
    BOOST_ASSERT(vote.is<T>());

    // Check if a voter is contained in a current voter set
    auto index_and_weight_opt = voter_set_->indexAndWeight(vote.id);
    if (!index_and_weight_opt) {
      SL_DEBUG(
          logger_, "Voter {} is not in the current voter set", vote.id.toHex());
      return VotingRoundError::UNKNOWN_VOTER;
    }
    const auto &[index, weight] = index_and_weight_opt.value();

    auto [type, type_str_, equivocators, tracker] =
        [&]() -> std::tuple<VoteType,
                            const char *const,
                            std::vector<bool> &,
                            VoteTracker &> {
      if constexpr (std::is_same_v<T, Prevote>) {
        return {
            VoteType::Prevote, "Prevote", prevote_equivocators_, *prevotes_};
      }
      if constexpr (std::is_same_v<T, Precommit>) {
        return {VoteType::Precommit,
                "Precommit",
                precommit_equivocators_,
                *precommits_};
      }
    }();
    auto &type_str = type_str_;  // Reference to binding for capturing in lambda

    // Ignore known equivocators
    if (equivocators[index]) {
      return VotingRoundError::VOTE_OF_KNOWN_EQUIVOCATOR;
    }

    // Ignore zero-weight voter
    if (weight == 0) {
      return VotingRoundError::ZERO_WEIGHT_VOTER;
    }

    auto push_res = tracker.push(vote, weight);
    switch (push_res) {
      case VoteTracker::PushResult::SUCCESS: {
        auto result = graph_->insert(type, vote.getBlockInfo(), vote.id);
        if (result.has_error()) {
          tracker.unpush(vote, weight);
          auto log_lvl = log::Level::WARN;
          if (result
              == outcome::failure(
                  blockchain::BlockTreeError::HEADER_NOT_FOUND)) {
            if (grandpa_context) {
              grandpa_context->missing_blocks.emplace(vote.getBlockInfo());
              log_lvl = log::Level::DEBUG;
            }
          }
          SL_LOG(logger_,
                 log_lvl,
                 "{} from {} for block {} was not inserted with error: {}",
                 type_str,
                 vote.id.toHex(),
                 vote.getBlockInfo(),
                 result.error());
          return result.as_failure();
        }
        save_historical_vote();
        return outcome::success();
      }
      case VoteTracker::PushResult::DUPLICATED: {
        return VotingRoundError::DUPLICATED_VOTE;
      }
      case VoteTracker::PushResult::EQUIVOCATED: {
        equivocators[index] = true;
        graph_->remove(type, vote.id);

        auto maybe_votes_opt = tracker.getMessage(vote.id);
        BOOST_ASSERT(maybe_votes_opt.has_value());
        const auto &votes_opt =
            if_type<EquivocatorySignedMessage>(maybe_votes_opt.value());
        BOOST_ASSERT(votes_opt.has_value());
        const auto &votes = votes_opt.value().get();

        Equivocation equivocation{round_number_, votes.first, votes.second};

        std::ignore = env_->reportEquivocation(*this, equivocation);

        save_historical_vote();
        return VotingRoundError::EQUIVOCATED_VOTE;
      }
      default:
        BOOST_UNREACHABLE_RETURN({});
    }
  }

  template outcome::result<void> VotingRoundImpl::onSigned<Prevote>(
      OptRef<GrandpaContext> grandpa_context, const SignedMessage &vote);

  template outcome::result<void> VotingRoundImpl::onSigned<Precommit>(
      OptRef<GrandpaContext> grandpa_context, const SignedMessage &vote);

  bool VotingRoundImpl::updateGrandpaGhost() {
    if (prevotes_->getTotalWeight() < threshold_) {
      SL_TRACE(logger_,
               "Round #{}: updateGrandpaGhost->false "
               "(total prevote weight={} < threshold={})",
               round_number_,
               prevotes_->getTotalWeight(),
               threshold_);
      return false;
    }

    auto current_best = prevBestFinalCandidate();

    auto possible_to_prevote = [this](const VoteWeight &weight) {
      return weight.total(VoteType::Prevote, prevote_equivocators_, *voter_set_)
          >= threshold_;
    };

    /// @see spec: Grandpa-Ghost
    auto new_prevote_ghost =
        graph_->findGhost(VoteType::Prevote, current_best, possible_to_prevote);

    if (new_prevote_ghost.has_value()) {
      bool changed = new_prevote_ghost != prevote_ghost_;

      prevote_ghost_ = new_prevote_ghost.value();

      if (changed) {
        SL_TRACE(logger_,
                 "Round #{}: updateGrandpaGhost->true "
                 "(prevote ghost was changed to block {})",
                 round_number_,
                 prevote_ghost_.value());
      } else {
        SL_TRACE(logger_,
                 "Round #{}: updateGrandpaGhost->false "
                 "(prevote ghost was not changed)",
                 round_number_);
      }
      return changed || new_prevote_ghost == last_finalized_block_;
    }

    SL_TRACE(logger_,
             "Round #{}: updateGrandpaGhost->false (no new prevote ghost)",
             round_number_);
    return false;
  }

  bool VotingRoundImpl::updateEstimate() {
    if (prevotes_->getTotalWeight() < threshold_) {
      SL_TRACE(logger_,
               "Round #{}: updateEstimate->false "
               "(total prevote weight={} < threshold={})",
               round_number_,
               prevotes_->getTotalWeight(),
               threshold_);
      return false;
    }

    if (not prevote_ghost_) {
      return false;
    }
    const auto &prevote_ghost = prevote_ghost_.value();

    // anything new finalized? finalized blocks are those which have both
    // 2/3+ prevote and precommit weight.
    if (precommits_->getTotalWeight() >= threshold_) {
      auto possible_to_finalize = [&](const VoteWeight &weight) {
        return weight.total(
                   VoteType::Precommit, precommit_equivocators_, *voter_set_)
            >= threshold_;
      };

      finalized_ = graph_->findAncestor(
          VoteType::Precommit, prevote_ghost, std::move(possible_to_finalize));

      BOOST_ASSERT(finalized_.has_value());
    }

    // find how many more equivocations we could still get.
    //
    // it is only important to consider the voters whose votes
    // we have already seen, because we are assuming any votes we
    // haven't seen will target this block.
    const auto tolerated_equivocations = voter_set_->totalWeight() - threshold_;

    // get total weight of all equivocators
    // NOLINTNEXTLINE(boost-use-ranges)
    const auto current_equivocations = std::accumulate(
        precommit_equivocators_.begin(),
        precommit_equivocators_.end(),
        0ul,
        [this, index = 0ul](size_t sum, auto isEquivocator) mutable -> size_t {
          if (not isEquivocator) {
            ++index;
            return sum;
          }
          return sum + voter_set_->voterWeight(index++).value();
        });

    const auto additional_equivocations =
        tolerated_equivocations - current_equivocations;

    const auto remaining_commit_votes =
        voter_set_->totalWeight() - precommits_->getTotalWeight();

    // figuring out whether a block can still be committed for is
    // not straightforward because we have to account for all possible future
    // equivocations and thus cannot discount weight from validators who
    // have already voted.
    auto possible_to_precommit = [&](const VoteWeight &weight) {
      // total precommits for this block, including equivocations.
      auto precommited_for = weight.total(
          VoteType::Precommit, precommit_equivocators_, *voter_set_);

      // equivocations we could still get are out of those who
      // have already voted, but not on this block.
      auto possible_equivocations =
          std::min<size_t>(precommits_->getTotalWeight() - precommited_for,
                           additional_equivocations);

      // all the votes already applied on this block,
      // assuming all remaining actors commit to this block,
      // and that we get further equivocations
      auto full_possible_weight =
          precommited_for + remaining_commit_votes + possible_equivocations;

      return full_possible_weight >= threshold_;
    };

    // until we have threshold precommits, any new block could get supermajority
    // precommits because there are at least f + 1 precommits remaining and then
    // f equivocations.
    //
    // once it's at least that level, we only need to consider blocks already
    // referenced in the graph, because no new leaf nodes could ever have enough
    // precommits.
    //
    // the round-estimate is the highest block in the chain with head
    // `prevote_ghost` that could have supermajority-commits.
    if (precommits_->getTotalWeight() < threshold_) {
      estimate_ = prevote_ghost;
      SL_TRACE(logger_,
               "Round #{}: updateEstimate->false: pc weight not enough => "
               "estimate=pv_ghost",
               round_number_);
      return false;
    }

    estimate_ = graph_->findAncestor(
        VoteType::Precommit, prevote_ghost, possible_to_precommit);

    if (not estimate_.has_value()) {
      completable_ = false;
      SL_TRACE(logger_,
               "Round #{}: updateEstimate: no estimate => completable=false",
               round_number_);
    } else {
      const auto &estimate = estimate_.value();

      if (estimate != prevote_ghost) {
        completable_ = true;
        SL_TRACE(
            logger_,
            "Round #{}: updateEstimate: estimate!=pv_ghost => completable=true",
            round_number_);
      } else {
        auto ghost_opt = graph_->findGhost(
            VoteType::Precommit, estimate, possible_to_precommit);

        if (not ghost_opt.has_value()) {
          completable_ = true;
          SL_TRACE(logger_,
                   "Round #{}: updateEstimate: no pc_ghost => completable=true",
                   round_number_);
        } else {
          const auto &ghost = ghost_opt.value();

          // round-estimate is the same as the prevote-ghost.
          // this round is still completable if no further blocks could have
          // commit-supermajority.

          if (ghost == estimate) {
            completable_ = true;
            SL_TRACE(logger_,
                     "Round #{}: updateEstimate: estimate==pc_ghost => "
                     "completable=true",
                     round_number_);
          } else {
            completable_ = false;
            SL_TRACE(logger_,
                     "Round #{}: updateEstimate: estimate!=pc_ghost => "
                     "completable=false",
                     round_number_);
          }
        }
      }
    }

    return true;
  }

  bool VotingRoundImpl::completable() const {
    return completable_;
  }

  BlockInfo VotingRoundImpl::bestFinalCandidate() {
    return estimate_.value_or(finalized_.value_or(last_finalized_block_));
  }

  MovableRoundState VotingRoundImpl::state() const {
    MovableRoundState round_state{.round_number = round_number_,
                                  .last_finalized_block = last_finalized_block_,
                                  .votes = prevotes_->getMessages(),
                                  .finalized = finalized_};

    auto precommits = precommits_->getMessages();
    round_state.votes.reserve(round_state.votes.size() + precommits.size());
    std::ranges::move(precommits, std::back_inserter(round_state.votes));

    return round_state;
  }

  std::vector<SignedPrevote> VotingRoundImpl::getPrevoteJustification(
      const BlockInfo &estimate, const std::vector<VoteVariant> &votes) const {
    // NOLINTNEXTLINE(boost-use-ranges)
    auto result = std::accumulate(
        votes.begin(),
        votes.end(),
        std::vector<SignedPrevote>(),
        [this, &estimate](auto &&prevotes, const auto &voting_variant) {
          visit_in_place(
              voting_variant,
              [this, &prevotes, &estimate](
                  const SignedMessage &voting_message) {
                if (voting_message.is<Prevote>()) {
                  if (env_->hasAncestry(estimate.hash,
                                        voting_message.getBlockHash())) {
                    prevotes.push_back(
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                        static_cast<const SignedPrevote &>(voting_message));
                  }
                }
              },
              [&prevotes](const EquivocatorySignedMessage
                              &equivocatory_voting_message) {
                prevotes.push_back(
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                    static_cast<const SignedPrevote &>(
                        equivocatory_voting_message.first));
                prevotes.push_back(
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                    static_cast<const SignedPrevote &>(
                        equivocatory_voting_message.second));
              });
          return std::move(prevotes);
        });
    return result;
  }

  std::vector<SignedPrecommit> VotingRoundImpl::getPrecommitJustification(
      const BlockInfo &estimate, const std::vector<VoteVariant> &votes) const {
    std::vector<SignedPrecommit> result;
    VoteWeight::Weight weight = 0;

    // Collect equivocations first (until threshold is reached)
    // NOLINTNEXTLINE(boost-use-ranges)
    result = std::accumulate(
        votes.begin(),
        votes.end(),
        std::move(result),
        [this, &weight](auto &&precommits, const auto &voting_variant) {
          if (weight < threshold_) {
            visit_in_place(
                voting_variant,
                [this, &precommits, &weight](const EquivocatorySignedMessage
                                                 &equivocatory_voting_message) {
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                  auto &signed_precommit = static_cast<const SignedPrecommit &>(
                      equivocatory_voting_message.first);
                  auto voter_weight =
                      voter_set_->voterWeight(signed_precommit.id);
                  if (voter_weight.has_value() and voter_weight.value() > 0) {
                    weight += voter_weight.value();
                    precommits.push_back(
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                        static_cast<const SignedPrecommit &>(
                            equivocatory_voting_message.first));
                    precommits.push_back(
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                        static_cast<const SignedPrecommit &>(
                            equivocatory_voting_message.second));
                  }
                },
                [](const auto &) {});
          }
          return std::move(precommits);
        });

    // Then collect valid precommits (until threshold is reached)
    // NOLINTNEXTLINE(boost-use-ranges)
    result = std::accumulate(
        votes.begin(),
        votes.end(),
        std::move(result),
        [this, &weight, &estimate](auto &&precommits,
                                   const auto &voting_variant) {
          if (weight < threshold_) {
            visit_in_place(
                voting_variant,
                [this, &precommits, &weight, &estimate](
                    const SignedMessage &voting_message) {
                  BOOST_ASSERT(voting_message.is<Precommit>());

                  if (estimate.number <= voting_message.getBlockNumber()
                      and env_->hasAncestry(estimate.hash,
                                            voting_message.getBlockHash())) {
                    auto &signed_precommit =
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                        static_cast<const SignedPrecommit &>(voting_message);
                    weight +=
                        voter_set_->voterWeight(signed_precommit.id).value();
                    precommits.push_back(signed_precommit);
                  }
                },
                [](const auto &) {});
          }
          return std::move(precommits);
        });

    return result;
  }

  void VotingRoundImpl::doCatchUpResponse(const libp2p::peer::PeerId &peer_id) {
    BOOST_ASSERT(finalized_.has_value());
    const auto &finalized_block = finalized_.value();

    auto estimate = estimate_.value_or(last_finalized_block_);
    auto prevote_justification =
        getPrevoteJustification(estimate, prevotes_->getMessages());

    auto precommit_justification =
        getPrecommitJustification(finalized_block, precommits_->getMessages());

    env_->onCatchUpRespond(peer_id,
                           voter_set_->id(),
                           round_number_,
                           std::move(prevote_justification),
                           std::move(precommit_justification),
                           finalized_block);
  }

  void VotingRoundImpl::sendNeighborMessage() {
    env_->onNeighborMessageSent(
        round_number_,
        voter_set_->id(),
        finalized_.value_or(last_finalized_block_).number);
  }

  void VotingRoundImpl::pending() {
    SL_DEBUG(logger_, "Round #{}: Pending…", round_number_);

    sendNeighborMessage();

    std::function<void(std::shared_ptr<VotingRound>)> resend =
        [&](std::shared_ptr<VotingRound> round_arg) mutable {
          auto round = std::dynamic_pointer_cast<VotingRoundImpl>(round_arg);
          BOOST_ASSERT_MSG(round != nullptr, "Expected single implementation");
          if (auto prev_round = round->getPreviousRound()) {
            resend(std::move(prev_round));
          }
          auto r = round->roundNumber();
          auto s = round->voterSetId();
          if (r == 0) {
            return;
          }
          SL_DEBUG(logger_, "Round #{}: resend votes", r);
          for (const auto &graph : {round->prevotes_, round->precommits_}) {
            for (const auto &vote_variant : graph->getMessages()) {
              visit_in_place(
                  vote_variant,
                  [&](const SignedMessage &vote) { env_->onVoted(r, s, vote); },
                  [&](const EquivocatorySignedMessage &pair) {
                    env_->onVoted(r, s, pair.first);
                    env_->onVoted(r, s, pair.second);
                  });
            }
          }
        };

    SL_DEBUG(logger_, "Resend votes of recent rounds");
    resend(shared_from_this());

    pending_timer_handle_ = scheduler_->scheduleWithHandle(
        [wself{weak_from_this()}] {
          if (auto self = wself.lock()) {
            self->pending();
          }
        },
        pending_interval_);
  }

  VotingRound::Votes VotingRoundImpl::votes() const {
    return {prevotes_->getMessages(), precommits_->getMessages()};
  }

  BlockInfo VotingRoundImpl::prevBestFinalCandidate() const {
    return previous_round_ ? previous_round_->bestFinalCandidate()
                           : last_finalized_block_;
  }
}  // namespace kagome::consensus::grandpa
