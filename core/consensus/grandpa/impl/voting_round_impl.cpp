/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/voting_round_impl.hpp"

#include <boost/range/adaptors.hpp>
#include <boost/range/numeric.hpp>
#include <unordered_map>
#include <unordered_set>

#include "common/visitor.hpp"
#include "consensus/grandpa/grandpa.hpp"
#include "consensus/grandpa/impl/voting_round_error.hpp"
#include "primitives/justification.hpp"

namespace kagome::consensus::grandpa {
  static auto convertToPrimaryPropose = [](const auto &vote) {
    return PrimaryPropose(vote.block_number, vote.block_hash);
  };

  static auto convertToBlockInfo = [](const auto &vote) {
    return BlockInfo(vote.block_number, vote.block_hash);
  };

  // based on
  // https://github.com/paritytech/finality-grandpa/blob/b19767c79adb17f20332672cb5f349206a864447/src/voter/voting_round.rs#L15

  VotingRoundImpl::VotingRoundImpl(
      const std::shared_ptr<Grandpa> &grandpa,
      const GrandpaConfig &config,
      std::shared_ptr<Environment> env,
      std::shared_ptr<VoteCryptoProvider> vote_crypto_provider,
      std::shared_ptr<VoteTracker> prevotes,
      std::shared_ptr<VoteTracker> precommits,
      std::shared_ptr<VoteGraph> graph,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<boost::asio::io_context> io_context)
      : grandpa_(grandpa),
        voter_set_{config.voters},
        round_number_{config.round_number},
        duration_{config.duration},
        id_{config.peer_id},
        env_{std::move(env)},
        vote_crypto_provider_{std::move(vote_crypto_provider)},
        prevotes_{std::move(prevotes)},
        precommits_{std::move(precommits)},
        graph_{std::move(graph)},
        clock_{std::move(clock)},
        io_context_{std::move(io_context)},
        timer_{*io_context_},
        pending_timer_{*io_context_},
        logger_{common::createLogger("Grandpa")},
        prevote_equivocators_(voter_set_->size(), false),
        precommit_equivocators_(voter_set_->size(), false) {
    BOOST_ASSERT(not grandpa_.expired());
    BOOST_ASSERT(voter_set_ != nullptr);
    BOOST_ASSERT(vote_crypto_provider_ != nullptr);
    BOOST_ASSERT(prevotes_ != nullptr);
    BOOST_ASSERT(precommits_ != nullptr);
    BOOST_ASSERT(env_ != nullptr);
    BOOST_ASSERT(graph_ != nullptr);
    BOOST_ASSERT(clock_ != nullptr);
    BOOST_ASSERT(not voter_set_->empty());

    // calculate supermajority
    threshold_ = [this] {
      auto faulty = (voter_set_->totalWeight() - 1) / 3;
      return voter_set_->totalWeight() - faulty;
    }();

    auto index = round_number_ % voter_set_->size();
    isPrimary_ = voter_set_->voters().at(index) == id_;

    logger_->debug("New round was created: #{}", round_number_);
  }

  VotingRoundImpl::VotingRoundImpl(
      const std::shared_ptr<Grandpa> &grandpa,
      const GrandpaConfig &config,
      const std::shared_ptr<Environment> &env,
      const std::shared_ptr<VoteCryptoProvider> &vote_crypto_provider,
      const std::shared_ptr<VoteTracker> &prevotes,
      const std::shared_ptr<VoteTracker> &precommits,
      const std::shared_ptr<VoteGraph> &graph,
      const std::shared_ptr<Clock> &clock,
      const std::shared_ptr<boost::asio::io_context> &io_context,
      const std::shared_ptr<VotingRound> &previous_round)
      : VotingRoundImpl(grandpa,
                        config,
                        env,
                        vote_crypto_provider,
                        prevotes,
                        precommits,
                        graph,
                        clock,
                        io_context) {
    BOOST_ASSERT(previous_round != nullptr);

    previous_round_ = previous_round;
    previous_round_state_ = previous_round->state();

    BOOST_ASSERT(previous_round_state_ != nullptr);

    constructCurrentState();
  }

  VotingRoundImpl::VotingRoundImpl(
      const std::shared_ptr<Grandpa> &grandpa,
      const GrandpaConfig &config,
      const std::shared_ptr<Environment> &env,
      const std::shared_ptr<VoteCryptoProvider> &vote_crypto_provider,
      const std::shared_ptr<VoteTracker> &prevotes,
      const std::shared_ptr<VoteTracker> &precommits,
      const std::shared_ptr<VoteGraph> &graph,
      const std::shared_ptr<Clock> &clock,
      const std::shared_ptr<boost::asio::io_context> &io_context,
      std::shared_ptr<const RoundState> previous_round_state)
      : VotingRoundImpl(grandpa,
                        config,
                        env,
                        vote_crypto_provider,
                        prevotes,
                        precommits,
                        graph,
                        clock,
                        io_context) {
    previous_round_state_ = std::move(previous_round_state);
    BOOST_ASSERT(previous_round_state_ != nullptr);

    constructCurrentState();
  }

  void VotingRoundImpl::constructCurrentState() {
    current_round_state_ = std::make_shared<RoundState>(RoundState{
        .last_finalized_block = previous_round_state_->finalized.value_or(
            previous_round_state_->last_finalized_block),
        .best_prevote_candidate = previous_round_state_->finalized.value_or(
            previous_round_state_->last_finalized_block),
        .best_final_candidate = previous_round_state_->finalized.value_or(
            previous_round_state_->last_finalized_block)});
  }

  void VotingRoundImpl::play() {
    if (stage_ != Stage::INIT) {
      return;
    }

    stage_ = Stage::START;

    logger_->debug("Start round #{}", round_number_);

    // Start pending mechanism
    pending();

    // Current local time (Tstart)
    start_time_ = clock_->now();

    // Derive-Primary
    // see ctor

    if (isPrimary_) {
      logger_->debug("Node is primary proposer at round #{}", round_number_);

      // Broadcast Fin-message with previous round best final candidate
      //  (or last finalized otherwise)
      // spec: Broadcast(M vr ¡1;Fin (Best-Final-Candidate(r-1)))
      if (auto previous_round = previous_round_.lock()) {
        previous_round->attemptToFinalizeRound();
      }

      // if Best-Final-Candidate greater than Last-Finalized-Block
      // spec: if Best-Final-Candidate(r ¡ 1) > Last-Finalized-Block
      if (previous_round_state_->best_final_candidate.block_number
          >= current_round_state_->last_finalized_block.block_number) {
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

    logger_->debug("Start prevote stage of round #{}", round_number_);

    // Continue to receive messages
    // until T>=Tstart + 2 * Duration or round is completable
    // spec: Receive-Messages(until Time>=Tr+2T or r is completable)

    if (completable()) {
      logger_->debug("Round #{} is already completable", round_number_);
      stage_ = Stage::PREVOTE_RUNS;
      endPrevoteStage();
      return;
    }

    timer_.cancel();
    timer_.expires_at(start_time_ + duration_ * 2);
    timer_.async_wait([this](const auto &...) {
      if (stage_ == Stage::PREVOTE_RUNS) {
        logger_->debug("Time of prevote stage of round #{} is out",
                       round_number_);
        endPrevoteStage();
      }
    });

    on_complete_handler_ = [this] {
      if (stage_ == Stage::PREVOTE_RUNS) {
        logger_->debug("Round #{} became completable", round_number_);
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

    stage_ = Stage::END_PREVOTE;

    // Broadcast vote for prevote stage
    doPrevote();

    logger_->debug("End prevote stage of round #{}", round_number_);

    timer_.cancel();
    on_complete_handler_ = nullptr;

    startPrecommitStage();
  }

  void VotingRoundImpl::startPrecommitStage() {
    if (stage_ == Stage::COMPLETED) {
      return;
    }
    BOOST_ASSERT(stage_ == Stage::END_PREVOTE);

    stage_ = Stage::START_PRECOMMIT;

    logger_->debug("Start precommit stage of round #{}", round_number_);

    // Continue to receive messages
    // until T>=Tstart + 4 * Duration or round is completable
    // spec: Receive-Messages(
    //  until Bpv>=Best-Final-Candidate(r-1)
    //  and (Time>=Tr+4T or r is completable)
    // )

    if (completable()) {
      logger_->debug("Round #{} is already completable", round_number_);
      stage_ = Stage::PRECOMMIT_RUNS;
      endPrecommitStage();
      return;
    }

    timer_.cancel();
    timer_.expires_at(start_time_ + duration_ * 4);
    timer_.async_wait([this](const auto &...) {
      if (stage_ == Stage::PRECOMMIT_RUNS) {
        logger_->debug("Time of precommit stage of round #{} is out",
                       round_number_);
        endPrecommitStage();
      }
    });

    on_complete_handler_ = [this] {
      if (stage_ == Stage::PRECOMMIT_RUNS) {
        logger_->debug("Round #{} became completable", round_number_);
        endPrecommitStage();
      }
    };

    stage_ = Stage::PRECOMMIT_RUNS;
  }

  void VotingRoundImpl::endPrecommitStage() {
    if (stage_ == Stage::COMPLETED) {
      return;
    }
    BOOST_ASSERT(stage_ == Stage::PRECOMMIT_RUNS);

    stage_ = Stage::END_PRECOMMIT;

    // Broadcast vote for precommit stage
    doPrecommit();

    logger_->debug("End precommit stage of round #{}", round_number_);

    timer_.cancel();
    on_complete_handler_ = nullptr;

    startWaitingStage();
  }

  void VotingRoundImpl::startWaitingStage() {
    if (stage_ == Stage::COMPLETED) {
      return;
    }
    BOOST_ASSERT(stage_ == Stage::END_PRECOMMIT);

    stage_ = Stage::START_WAITING;

    logger_->debug("Start final stage of round #{}", round_number_);

    // Continue to receive messages until current round is completable and
    // previous one is finalisable and last filanized better than best filan
    // candidate of prefious round

    // spec: Receive-Messages(until r is completable and Finalizable(r ¡ 1) and
    // Last-Finalized-Block>Best-Final-Candidate(r-1))

    bool isReadyToEnd =
        completable_ && previous_round_state_->finalized
        && previous_round_state_->finalized.get().block_number
               >= previous_round_state_->best_final_candidate.block_number;

    if (isReadyToEnd) {
      logger_->debug("Conditions for final stage of round #{} already met",
                     round_number_);
      stage_ = Stage::WAITING_RUNS;
      endWaitingStage();
      return;
    }

    auto handle_end_condition = [this] {
      bool isReadyToEnd =
          completable_ && previous_round_state_->finalized
          && previous_round_state_->finalized.get().block_number
                 >= previous_round_state_->best_final_candidate.block_number;

      if (isReadyToEnd) {
        logger_->debug("Conditions for final stage of round #{} are met",
                       round_number_);
        endWaitingStage();
      }
    };

    on_complete_handler_ = handle_end_condition;

    stage_ = Stage::WAITING_RUNS;

    attemptToFinalizeRound();
  }

  void VotingRoundImpl::endWaitingStage() {
    if (stage_ == Stage::COMPLETED) {
      return;
    }
    BOOST_ASSERT(stage_ == Stage::WAITING_RUNS);

    stage_ = Stage::END_WAITING;

    logger_->debug("End final stage of round #{}", round_number_);

    // Reset handler of previous round finalizable
    on_complete_handler_ = nullptr;

    // Stop pending
    pending_timer_.cancel();

    // Play new round
    // spec: Play-Grandpa-round(r + 1);

    if (auto grandpa = grandpa_.lock()) {
      grandpa->executeNextRound();
    }
  }

  void VotingRoundImpl::end() {
    stage_ = Stage::COMPLETED;
    on_complete_handler_ = nullptr;
    timer_.cancel();
    pending_timer_.cancel();
  }

  void VotingRoundImpl::doProposal() {
    // Send primary propose
    // @spec Broadcast(M vr ¡1;Prim (Best-Final-Candidate(r-1)))

    BOOST_ASSERT_MSG(not primary_vote_.has_value(),
                     "Primary proposal must be once for a round");
    primary_vote_ =
        convertToPrimaryPropose(previous_round_state_->best_final_candidate);

    sendProposal(primary_vote_.value());
  }

  void VotingRoundImpl::sendProposal(const PrimaryPropose &primary_proposal) {
    auto signed_primary_proposal =
        vote_crypto_provider_->signPrimaryPropose(primary_proposal);

    logger_->debug("Propose block #{} hash={} in round #{}",
                   signed_primary_proposal.block_number(),
                   signed_primary_proposal.block_hash(),
                   round_number_);

    auto proposed = env_->onProposed(
        round_number_, voter_set_->id(), signed_primary_proposal);

    if (not proposed) {
      logger_->error("Primary proposal was not sent in round #{}: {}",
                     round_number_,
                     proposed.error().message());
    }
  }

  void VotingRoundImpl::doPrevote() {
    BOOST_ASSERT_MSG(not prevote_.has_value(),
                     "Prevote must be once for a round");

    auto previous_round = previous_round_.lock();

    // spec: L <- Best-Final-Candidate(r-1)
    const auto &best_final_candidate =
        previous_round ? previous_round->bestFinalCandidate()
                       : previous_round_state_->best_final_candidate;
    last_finalized_block_ = best_final_candidate;

    // spec: N <- Best-PreVote-Candidate(r)
    const auto best_prevote_candidate = bestPrevoteCandidate();
    current_round_state_->best_prevote_candidate = best_prevote_candidate;

    prevote_ = {{best_prevote_candidate.block_number,
                 best_prevote_candidate.block_hash}};

    // Broadcast vote for prevote stage
    // spec: Broadcast(Bpv)
    sendPrevote(prevote_.value());
  }

  void VotingRoundImpl::sendPrevote(const Prevote &prevote) {
    logger_->debug("Prevote block #{} hash={} in round #{}",
                   prevote.block_number,
                   prevote.block_hash,
                   round_number_);

    auto signed_prevote = vote_crypto_provider_->signPrevote(prevote);
    auto prevoted =
        env_->onPrevoted(round_number_, voter_set_->id(), signed_prevote);

    if (not prevoted) {
      logger_->error("Prevote was not sent: {}", prevoted.error().message());
    }
  }

  void VotingRoundImpl::doPrecommit() {
    BOOST_ASSERT_MSG(not precommit_.has_value(),
                     "Precommit must be once for a round");

    auto previous_round = previous_round_.lock();
    auto last_round_estimate =
        previous_round ? previous_round->bestFinalCandidate()
                       : previous_round_state_->best_final_candidate;

    auto best_prevote_candidate = bestPrevoteCandidate();
    current_round_state_->best_prevote_candidate = best_prevote_candidate;

    // We should precommit if current state contains prevote and it is
    // either equal to the last round estimate or is descendant of it
    bool should_precommit =
        best_prevote_candidate == last_round_estimate
        or env_->isEqualOrDescendOf(last_round_estimate.block_hash,
                                    best_prevote_candidate.block_hash);

    if (should_precommit) {
      precommit_ = {{best_prevote_candidate.block_number,
                     best_prevote_candidate.block_hash}};
      sendPrecommit(precommit_.value());
      return;
    }

    env_->onCompleted(VotingRoundError::LAST_ESTIMATE_BETTER_THAN_PREVOTE);
  }

  void VotingRoundImpl::sendPrecommit(const Precommit &precommit) {
    auto signed_precommit = vote_crypto_provider_->signPrecommit(precommit);

    logger_->debug("Precommit block #{} hash={} in round #{}",
                   precommit.block_number,
                   precommit.block_hash,
                   round_number_);

    auto precommited =
        env_->onPrecommitted(round_number_, voter_set_->id(), signed_precommit);

    if (not precommited) {
      logger_->error("Precommit was not sent: {}",
                     precommited.error().message());
    }
  }

  void VotingRoundImpl::doFinalize() {
    BOOST_ASSERT(finalizable());
    BOOST_ASSERT(current_round_state_->finalized.has_value());

    auto block = current_round_state_->finalized.value();
    const auto &justification_opt =
        getJustification(block, precommits_->getMessages());
    if (not justification_opt) {
      logger_->warn("No justification for block  <{}, {}>",
                    block.block_number,
                    block.block_hash.toHex());
    }
    auto &justification = justification_opt.value();

    sendFinalize(block, justification);
  }

  void VotingRoundImpl::sendFinalize(
      const BlockInfo &block, const GrandpaJustification &justification) {
    logger_->debug("Finalize block #{} hash={} in round #{}",
                   block.block_number,
                   block.block_hash.toHex(),
                   round_number_);

    auto finalized = env_->onCommitted(round_number_, block, justification);
    if (not finalized) {
      logger_->error("Finalize was not sent: {}", finalized.error().message());
    }
  }

  bool VotingRoundImpl::isPrimary(const Id &id) const {
    auto index = round_number_ % voter_set_->size();
    return voter_set_->voters().at(index) == id;
  }

  bool VotingRoundImpl::isPrimary() const {
    return isPrimary(id_);
  }

  size_t VotingRoundImpl::getThreshold(
      const std::shared_ptr<VoterSet> &voters) {
    // calculate supermajority
    auto faulty = (voters->totalWeight() - 1) / 3;
    return voters->totalWeight() - faulty;
  }

  void VotingRoundImpl::onFinalize(const Fin &finalize) {
    // validate message
    auto result =
        validatePrecommitJustification(finalize.vote, finalize.justification);
    if (not result) {
      logger_->error(
          "Finalisation of round #{} is received for block #{} hash={} was "
          "rejected: validation failed",
          round_number_,
          finalize.vote.block_number,
          finalize.vote.block_hash.toHex());
      env_->onCompleted(VotingRoundError::FIN_VALIDATION_FAILED);
      return;
    }

    // finalize to state
    auto finalized =
        env_->finalize(finalize.vote.block_hash, finalize.justification);
    if (not finalized) {
      logger_->debug(
          "Finalisation of round #{} is received for block #{} hash={} with "
          "error: {}",
          round_number_,
          finalize.vote.block_number,
          finalize.vote.block_hash.toHex(),
          finalized.error().message());
      return;
    }

    logger_->debug(
        "Finalisation of round #{} is received for block #{} hash={}",
        round_number_,
        finalize.vote.block_number,
        finalize.vote.block_hash.toHex());

    env_->onCompleted(CompletedRound{.round_number = round_number_,
                                     .state = current_round_state_});
  }

  bool VotingRoundImpl::validatePrevoteJustification(
      const BlockInfo &vote, const GrandpaJustification &justification) const {
    size_t total_weight = 0;

    std::unordered_map<Id, BlockHash> validators;
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
        logger_->error(
            "Received invalid signed precommit during the round {} from the "
            "peer {}",
            round_number_,
            signed_precommit.id.toHex());
        return false;
      }

      // check that every signed precommit corresponds to the vote (i.e.
      // signed_precommits are descendants of the vote). If so add weight of
      // that voter to the total weight

      if (auto [it, success] = validators.emplace(
              signed_precommit.id, signed_precommit.block_hash());
          success) {
        // New vote
        if (env_->getAncestry(vote.block_hash, signed_precommit.block_hash())) {
          total_weight += voter_set_->voterWeight(signed_precommit.id).value();
        }

      } else if (equivocators.emplace(signed_precommit.id).second) {
        // Detected equivocation
        if (env_->getAncestry(vote.block_hash, it->second)) {
          total_weight -= voter_set_->voterWeight(signed_precommit.id).value();
        }

      } else {
        // Detected duplicate of equivotation
        logger_->error(
            "Received third precommit of caught equivocator during the round "
            "{} from the peer {}",
            round_number_,
            signed_precommit.id.toHex());
        return false;
      }
    }

    return total_weight >= threshold_;
  }

  bool VotingRoundImpl::validatePrecommitJustification(
      const BlockInfo &vote, const GrandpaJustification &justification) const {
    size_t total_weight = 0;

    std::unordered_map<Id, BlockHash> validators;
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
        logger_->error(
            "Received invalid signed precommit during the round {} from the "
            "peer {}",
            round_number_,
            signed_precommit.id.toHex());
        return false;
      }

      // check that every signed precommit corresponds to the vote (i.e.
      // signed_precommits are descendants of the vote). If so add weight of
      // that voter to the total weight

      if (auto [it, success] = validators.emplace(
              signed_precommit.id, signed_precommit.block_hash());
          success) {
        // New vote
        if (env_->getAncestry(vote.block_hash, signed_precommit.block_hash())) {
          total_weight += voter_set_->voterWeight(signed_precommit.id).value();
        }

      } else if (equivocators.emplace(signed_precommit.id).second) {
        // Detected equivocation
        if (env_->getAncestry(vote.block_hash, it->second)) {
          total_weight -= voter_set_->voterWeight(signed_precommit.id).value();
        }

      } else {
        // Detected duplicate of equivotation
        logger_->error(
            "Received third precommit of caught equivocator during the round "
            "{} from the peer {}",
            round_number_,
            signed_precommit.id.toHex());
        return false;
      }
    }

    return total_weight >= threshold_;
  }

  void VotingRoundImpl::attemptToFinalizeRound() {
    if (stage_ != Stage::WAITING_RUNS) {
      return;
    }

    if (finalizable()) {
      doFinalize();
      return;
    }

    if (logger_->level() >= spdlog::level::level_enum::debug) {
      if (not completable_) {
        logger_->debug(
            "Round #{} did not finalized. "
            "Reason: not completable",
            round_number_);
      } else if (not current_round_state_->finalized.has_value()) {
        logger_->debug(
            "Round #{} did not finalized. "
            "Reason: not finalizable",
            round_number_);
      } else if (current_round_state_->finalized
                 == previous_round_state_->finalized) {
        logger_->debug(
            "Round #{} did not finalized. "
            "Reason: new state is equal to the previous one",
            round_number_);
      } else {
        logger_->debug(
            "Round #{} did not finalized. "
            "Reason: unknown",
            round_number_);
      }
    }
  }

  outcome::result<void> VotingRoundImpl::notify() {
    BOOST_ASSERT(completable_);
    BOOST_ASSERT(current_round_state_->finalized.has_value());
    //    BOOST_ASSERT(current_round_state_->finalized ==
    //    previous_round_state_->finalized);

    auto finalized = current_round_state_->finalized.value();
    const auto &justification_opt =
        getJustification(finalized, precommits_->getMessages());
    if (not justification_opt) {
      logger_->warn("No justification for block  <{}, {}>",
                    finalized.block_number,
                    finalized.block_hash.toHex());
    }

    auto justification = justification_opt.value();

    OUTCOME_TRY(env_->onCommitted(round_number_, finalized, justification));

    return outcome::success();
  }

  RoundNumber VotingRoundImpl::roundNumber() const {
    return round_number_;
  }

  MembershipCounter VotingRoundImpl::voterSetId() const {
    return voter_set_->id();
  }

  std::shared_ptr<const RoundState> VotingRoundImpl::state() const {
    return current_round_state_;
  }

  void VotingRoundImpl::onProposal(const SignedMessage &proposal) {
    if (not isPrimary(proposal.id)) {
      logger_->warn("Round #{} proposal received from {} that is not primary",
                    round_number_,
                    proposal.id.toHex());
      return;
    }

    bool isValid = vote_crypto_provider_->verifyPrimaryPropose(proposal);
    if (not isValid) {
      logger_->warn("Round #{} proposal received from {} has invalid signature",
                    round_number_,
                    proposal.id.toHex());
      return;
    }

    primary_vote_ = {{proposal.block_number(), proposal.block_hash()}};
  }

  void VotingRoundImpl::onPrevote(const SignedMessage &prevote) {
    bool isValid = vote_crypto_provider_->verifyPrevote(prevote);
    if (not isValid) {
      logger_->warn("Round #{} prevote received from {} has invalid signature",
                    round_number_,
                    prevote.id.toHex());
      return;
    }

    auto result = onSignedPrevote(prevote);
    if (result.has_failure()) {
      if (result != outcome::failure(VotingRoundError::DUPLICATED_VOTE)) {
        logger_->warn("Round #{} prevote received from {} was rejected: {}",
                      round_number_,
                      prevote.id.toHex(),
                      result.error().message());
        return;
      }
    }

    logger_->debug("Round #{} prevote received from {}",
                   round_number_,
                   prevote.id.toHex());

    updatePrevoteGhost();
    update();
    attemptToFinalizeRound();
  }

  void VotingRoundImpl::onPrecommit(const SignedMessage &precommit) {
    bool isValid = vote_crypto_provider_->verifyPrecommit(precommit);
    if (not isValid) {
      logger_->warn(
          "Round #{} precommit received from {} has invalid signature",
          round_number_,
          precommit.id.toHex());
      return;
    }

    auto result = onSignedPrecommit(precommit);
    if (result.has_failure()) {
      if (result != outcome::failure(VotingRoundError::DUPLICATED_VOTE)) {
        logger_->warn("Round #{} precommit received from {} was rejected: {}",
                      round_number_,
                      precommit.id.toHex(),
                      result.error().message());
        env_->onCompleted(result.as_failure());
        return;
      }
    }

    logger_->debug("Round #{} precommit received from {}",
                   round_number_,
                   precommit.id.toHex());

    update();
    attemptToFinalizeRound();
  }

  outcome::result<void> VotingRoundImpl::onSignedPrevote(
      const SignedMessage &vote) {
    BOOST_ASSERT(vote.is<Prevote>());
    auto weight = voter_set_->voterWeight(vote.id);
    if (not weight.has_value()) {
      logger_->warn("Voter {} is not known: {}", vote.id.toHex());
      return VotingRoundError::UNKNOWN_VOTER;
    }
    auto index = voter_set_->voterIndex(vote.id);
    if (not index) {
      BOOST_ASSERT_MSG(
          false,
          "Can't be none after voterWeight() was succeed");  // NOLINTNEXTLINE
      return VotingRoundError::UNKNOWN_VOTER;
    }

    switch (prevotes_->push(vote, weight.value())) {
      case VoteTracker::PushResult::SUCCESS: {
        const auto &voters = voter_set_->voters();

        // prepare VoteWeight which contains index of who has voted and what
        // kind of vote it was
        VoteWeight voteWeight{voters.size()};

        voteWeight.setPrevote(index.value(),
                              voter_set_->voterWeight(vote.id).value());

        auto result = graph_->insert(vote.message, voteWeight);
        if (not result.has_value()) {
          logger_->warn("Prevote {} was not inserted with error: {}",
                        vote.block_hash().toHex(),
                        result.error().message());
          return result.as_failure();
        }
        return outcome::success();
      }
      case VoteTracker::PushResult::DUPLICATED: {
        return VotingRoundError::DUPLICATED_VOTE;
      }
      case VoteTracker::PushResult::EQUIVOCATED: {
        prevote_equivocators_[index.value()] = true;
        return VotingRoundError::EQUIVOCATED_VOTE;
      }
    }
  }

  outcome::result<void> VotingRoundImpl::onSignedPrecommit(
      const SignedMessage &vote) {
    BOOST_ASSERT(vote.is<Precommit>());
    auto weight = voter_set_->voterWeight(vote.id);
    if (not weight.has_value()) {
      logger_->warn("Voter {} is not known: {}", vote.id.toHex());
      return VotingRoundError::UNKNOWN_VOTER;
    }
    auto index = voter_set_->voterIndex(vote.id);
    if (not index) {
      BOOST_ASSERT_MSG(false, "Can't be none after voterWeight() was succeed");
      return VotingRoundError::UNKNOWN_VOTER;
    }

    switch (precommits_->push(vote, weight.value())) {
      case VoteTracker::PushResult::SUCCESS: {
        const auto &voters = voter_set_->voters();

        // prepare VoteWeight which contains index of who has voted and what
        // kind of vote it was
        VoteWeight voteWeight{voters.size()};

        voteWeight.setPrecommit(index.value(),
                                voter_set_->voterWeight(vote.id).value());

        auto result = graph_->insert(vote.message, voteWeight);
        if (not result.has_value()) {
          logger_->warn("Precommit {} was not inserted with error: {}",
                        vote.block_hash().toHex(),
                        result.error().message());
          return result.as_failure();
        }
        return outcome::success();
      }
      case VoteTracker::PushResult::DUPLICATED: {
        return VotingRoundError::DUPLICATED_VOTE;
      }
      case VoteTracker::PushResult::EQUIVOCATED: {
        precommit_equivocators_[index.value()] = true;
        return VotingRoundError::EQUIVOCATED_VOTE;
      }
    }
    return outcome::success();
  }

  BlockInfo VotingRoundImpl::grandpaGhost() {
    BlockInfo current_best =
        current_round_state_->best_prevote_candidate.block_number
                > current_round_state_->last_finalized_block.block_number
            ? current_round_state_->best_prevote_candidate
            : current_round_state_->last_finalized_block;

    auto prevote_ghost = graph_->findGhost(
        current_best,
        [](const auto &) { return true; },
        VoteWeight::prevoteComparator);

    if (prevote_ghost.has_value()) {
      current_round_state_->best_prevote_candidate = prevote_ghost.value();
    }

    return current_round_state_->best_prevote_candidate;
  }

  void VotingRoundImpl::updatePrevoteGhost() {
    if (prevotes_->getTotalWeight() < threshold_) {
      return;
    }

    boost::optional<BlockInfo> ghost_block_info;
    if (current_round_state_->best_prevote_candidate
        != current_round_state_->last_finalized_block) {
      ghost_block_info.emplace(
          convertToBlockInfo(current_round_state_->best_prevote_candidate));
    }

    auto posible_to_prevote = [this](const VoteWeight &vote_weight) {
      return vote_weight
                 .totalWeight(
                     prevote_equivocators_, precommit_equivocators_, voter_set_)
                 .prevote
             >= threshold_;
    };

    auto prevote_ghost = graph_->findGhost(
        ghost_block_info, posible_to_prevote, VoteWeight::prevoteComparator);

    if (prevote_ghost.has_value()) {
      current_round_state_->best_prevote_candidate = prevote_ghost.value();
    }
  }

  bool VotingRoundImpl::completable() const {
    return completable_;
  }

  bool VotingRoundImpl::finalizable() const {
    return completable_ && (current_round_state_->finalized.has_value() || finalized_.has_value()) ;
  }

  BlockInfo VotingRoundImpl::bestPrevoteCandidate() {
    auto previous_round = previous_round_.lock();

    // spec: L <- Best-Final-Candidate(r-1)
    auto best_final_candicate =
        previous_round ? previous_round->bestFinalCandidate()
                       : previous_round_state_->best_final_candidate;

    // spec: Bpv <- GRANDPA-GHOST(r)
    auto best_prevote_candidate = grandpaGhost();

    // spec: if Received(Bprim) and Bpv >= Bprim > L
    if (primary_vote_.has_value()) {
      auto &primary = primary_vote_.value();

      if (best_prevote_candidate.block_number >= primary.block_number
          and primary.block_number > best_final_candicate.block_number) {
        // spec: N <- Bprim
        return {primary.block_number, primary.block_hash};
      }
    }

    // spec: N <- Bpv
    return best_prevote_candidate;
  }

  BlockInfo VotingRoundImpl::bestFinalCandidate() {
    auto current_precommits = precommits_->getTotalWeight();

    const auto &best_prevote_candidate = bestPrevoteCandidate();
    current_round_state_->best_prevote_candidate = best_prevote_candidate;

    if (current_precommits >= threshold_) {
      auto possible_to_finalize = [this](const VoteWeight &vote_weight) {
        return vote_weight
                   .totalWeight(prevote_equivocators_,
                                precommit_equivocators_,
                                voter_set_)
                   .precommit
               >= threshold_;
      };

      current_round_state_->finalized = finalized_ =
          graph_->findAncestor(best_prevote_candidate,
                               possible_to_finalize,
                               VoteWeight::precommitComparator);
    }

    // figuring out whether a block can still be committed for is
    // not straightforward because we have to account for all possible future
    // equivocations and thus cannot discount weight from validators who
    // have already voted.
    auto tolerated_equivocations = voter_set_->totalWeight() - threshold_;

    // find how many more equivocations we could still get.
    //
    // it is only important to consider the voters whose votes
    // we have already seen, because we are assuming any votes we
    // haven't seen will target this block.

    // get total weight of all equivocators
    using namespace boost::adaptors;  // NOLINT
    auto current_equivocations = boost::accumulate(
        voter_set_->voters() | indexed(0)
            | transformed([this](const auto &element) {
                return precommit_equivocators_[element.index()]
                           ? voter_set_->voterWeight(element.index()).value()
                           : 0UL;
              }),
        0UL);

    auto additional_equiv = tolerated_equivocations - current_equivocations;
    auto possible_to_precommit = [&](const VoteWeight &weight) {
      // total precommits for this block, including equivocations.
      auto precommited_for =
          weight
              .totalWeight(
                  prevote_equivocators_, precommit_equivocators_, voter_set_)
              .precommit;

      // equivocations we could still get are out of those who
      // have already voted, but not on this block.
      auto possible_equivocations = std::min<size_t>(
          current_precommits - precommited_for, additional_equiv);

      auto remaining_commit_votes =
          voter_set_->totalWeight() - precommits_->getTotalWeight();

      // all the votes already applied on this block,
      // assuming all remaining actors commit to this block,
      // and that we get further equivocations
      auto full_possible_weight =
          precommited_for + remaining_commit_votes + possible_equivocations;

      return full_possible_weight >= threshold_;
    };

    // until we have threshold precommits, any new block could get
    // supermajority precommits because there are at least f + 1 precommits
    // remaining and then f equivocations.
    //
    // once it's at least that level, we only need to consider blocks
    // already referenced in the graph, because no new leaf nodes
    // could ever have enough precommits.
    //
    // the round-estimate is the highest block in the chain with head
    // `prevote_ghost` that could have supermajority-commits.
    if (precommits_->getTotalWeight() >= threshold_) {
      auto block = graph_->findAncestor(best_prevote_candidate,
                                        possible_to_precommit,
                                        VoteWeight::precommitComparator);
      if (block.has_value()) {
        current_round_state_->best_final_candidate = best_final_candidate_ =
            block.value();
      }
    } else {
      current_round_state_->best_final_candidate = best_final_candidate_ =
          best_prevote_candidate;
      return best_final_candidate_;
    }

    const auto &block = current_round_state_->best_final_candidate =
        best_final_candidate_;
    bool maybe_need_handle_completability = not completable_;
    completable_ =
        block.block_hash != best_prevote_candidate.block_hash
        || graph_
               ->findGhost(block,
                           possible_to_precommit,
                           VoteWeight::precommitComparator)
               .map([&best_prevote_candidate](const BlockInfo &block_info) {
                 return block_info == best_prevote_candidate;
               })
               .value_or(true);
    if (maybe_need_handle_completability && completable_
        && on_complete_handler_) {
      on_complete_handler_();
    }

    return best_final_candidate_;
  }

  outcome::result<Prevote> VotingRoundImpl::calculatePrevote() const {
    const auto &last_round_estimate =
        previous_round_state_->best_final_candidate;

    // find the block that we should take descendent from
    auto find_descendent_of =
        primary_vote_.map(convertToBlockInfo)
            .map([&](const BlockInfo &primary) {
              // if we have primary_vote then:
              // if last prevote is the same as primary vote, then return it
              // else if primary vote is better than last prevote return last
              // round estimate
              auto last_prevote_g =
                  previous_round_state_->best_prevote_candidate;

              if (std::tie(primary.block_number, primary.block_hash)
                  == std::tie(last_prevote_g.block_number,
                              last_prevote_g.block_hash)) {
                return primary;
              }
              if (primary.block_number >= last_prevote_g.block_number) {
                return last_round_estimate;
              }

              // from this point onwards, the number of the
              // primary-broadcasted block is less than the last
              // prevote-GHOST's number. if the primary block is in the
              // ancestry of p-G we vote for the best chain containing it.
              if (auto ancestry =
                      env_->getAncestry(last_round_estimate.block_hash,
                                        last_prevote_g.block_hash);
                  ancestry) {
                auto to_sub = primary.block_number + 1;

                primitives::BlockNumber offset = 0;
                if (last_prevote_g.block_number >= to_sub) {
                  offset = last_prevote_g.block_number - to_sub;
                }

                if (ancestry.value().at(offset) == primary.block_hash) {
                  return primary;
                }
              }
              return last_round_estimate;
            })
            .value_or_eval(
                [last_round_estimate] { return last_round_estimate; });

    auto rbest_chain = env_->bestChainContaining(find_descendent_of.block_hash);

    if (not rbest_chain) {
      logger_->error(
          "Could not cast prevote: previously known block {} has disappeared",
          find_descendent_of.block_hash.toHex());
      return outcome::failure(boost::system::error_code());
    }
    const auto &best_chain = rbest_chain.value();

    return {best_chain.block_number, best_chain.block_hash};
  }

  void VotingRoundImpl::update() {
    // No supermajority in prevotes
    if (prevotes_->getTotalWeight() < threshold_) {
      return;
    }

    const auto &best_prevote_candidate =
        current_round_state_->best_prevote_candidate;

    // anything new finalized? finalized blocks are those which have both
    // 2/3+ precommit weight.
    auto current_precommits = precommits_->getTotalWeight();

    if (current_precommits >= threshold_) {
      auto possible_to_finalize = [this](const VoteWeight &vote_weight) {
        return vote_weight
                   .totalWeight(prevote_equivocators_,
                                precommit_equivocators_,
                                voter_set_)
                   .precommit
               >= threshold_;
      };

	    finalized_ =
	    		current_round_state_->finalized =
          graph_->findAncestor(best_prevote_candidate,
                               possible_to_finalize,
                               VoteWeight::precommitComparator);
    }

    // figuring out whether a block can still be committed for is
    // not straightforward because we have to account for all possible future
    // equivocations and thus cannot discount weight from validators who
    // have already voted.
    auto tolerated_equivocations = voter_set_->totalWeight() - threshold_;

    // find how many more equivocations we could still get.
    //
    // it is only important to consider the voters whose votes
    // we have already seen, because we are assuming any votes we
    // haven't seen will target this block.

    // get total weight of all equivocators
    using namespace boost::adaptors;  // NOLINT
    auto current_equivocations = boost::accumulate(
        voter_set_->voters() | indexed(0)
            | transformed([this](const auto &element) {
                return precommit_equivocators_[element.index()]
                           ? voter_set_->voterWeight(element.index()).value()
                           : 0UL;
              }),
        0UL);

    auto additional_equiv = tolerated_equivocations - current_equivocations;
    auto possible_to_precommit = [&](const VoteWeight &weight) {
      // total precommits for this block, including equivocations.
      auto precommited_for =
          weight
              .totalWeight(
                  prevote_equivocators_, precommit_equivocators_, voter_set_)
              .precommit;

      // equivocations we could still get are out of those who
      // have already voted, but not on this block.
      auto possible_equivocations = std::min<size_t>(
          current_precommits - precommited_for, additional_equiv);

      auto remaining_commit_votes =
          voter_set_->totalWeight() - precommits_->getTotalWeight();

      // all the votes already applied on this block,
      // assuming all remaining actors commit to this block,
      // and that we get further equivocations
      auto full_possible_weight =
          precommited_for + remaining_commit_votes + possible_equivocations;
      return full_possible_weight >= threshold_;
    };

    // until we have threshold precommits, any new block could get
    // supermajority precommits because there are at least f + 1 precommits
    // remaining and then f equivocations.
    //
    // once it's at least that level, we only need to consider blocks
    // already referenced in the graph, because no new leaf nodes
    // could ever have enough precommits.
    //
    // the round-estimate is the highest block in the chain with head
    // `prevote_ghost` that could have supermajority-commits.
    if (precommits_->getTotalWeight() >= threshold_) {
      auto block = graph_->findAncestor(best_prevote_candidate,
                                        possible_to_precommit,
                                        VoteWeight::precommitComparator);
      if (block.has_value()) {
        current_round_state_->best_final_candidate = block.value();
      }
    } else {
      current_round_state_->best_final_candidate = best_prevote_candidate;
      return;
    }

    const auto &block = current_round_state_->best_final_candidate;
    bool maybe_need_handle_completability = not completable_;
    completable_ =
        block.block_hash != best_prevote_candidate.block_hash
        || graph_
               ->findGhost(block,
                           possible_to_precommit,
                           VoteWeight::precommitComparator)
               .map([&best_prevote_candidate](const BlockInfo &block_info) {
                 return block_info == best_prevote_candidate;
               })
               .value_or(true);
    if (maybe_need_handle_completability && completable_
        && on_complete_handler_) {
      on_complete_handler_();
    }
  }

  boost::optional<GrandpaJustification> VotingRoundImpl::getJustification(
      const BlockInfo &estimate, const std::vector<VoteVariant> &votes) const {
    GrandpaJustification result = std::accumulate(
        votes.begin(),
        votes.end(),
        GrandpaJustification{},
        [this, &estimate](GrandpaJustification &justification,
                          const auto &voting_variant) {
          visit_in_place(
              voting_variant,
              [this, &justification, &estimate](
                  const SignedMessage &voting_message) {
                if (env_->isEqualOrDescendOf(estimate.block_hash,
                                             voting_message.block_hash())) {
                  justification.items.push_back(voting_message);
                }
              },
              [&justification](const EquivocatoryVotingMessage
                                   &equivocatory_voting_message) {
                justification.items.push_back(
                    equivocatory_voting_message.first);
                justification.items.push_back(
                    equivocatory_voting_message.second);
              });
          return justification;
        });
    return result;
  }

  void VotingRoundImpl::doCatchUpRequest(const libp2p::peer::PeerId &peer_id) {
    // TODO(xDimon): Check if peer is known validator
    //  	if (not voter_set_->voterIndex(peer_id).has_value()) {
    //		  return;
    //	  }
    auto res =
        env_->onCatchUpRequested(peer_id, voter_set_->id(), round_number_);
    if (not res) {
      logger_->warn("Catch-Up-Request was not sent: {}", res.error().message());
    }
  }

  void VotingRoundImpl::doCatchUpResponse(const libp2p::peer::PeerId &peer_id) {
    BOOST_ASSERT(current_round_state_->finalized.has_value());
    const auto &finalised_block = current_round_state_->finalized.value();

    auto best_prevote_candidate = bestPrevoteCandidate();
    auto prevote_justification_opt =
        getJustification(best_prevote_candidate, prevotes_->getMessages());
    BOOST_ASSERT(prevote_justification_opt.has_value());
    auto &prevote_justification = prevote_justification_opt.value();

    auto best_final_candidate = bestFinalCandidate();
    auto precommit_justification_opt =
        getJustification(best_final_candidate, precommits_->getMessages());
    BOOST_ASSERT(precommit_justification_opt.has_value());
    auto &precommit_justification = precommit_justification_opt.value();

    auto result = env_->onCatchUpResponsed(peer_id,
                                           voter_set_->id(),
                                           round_number_,
                                           std::move(prevote_justification),
                                           std::move(precommit_justification),
                                           finalised_block);
    if (not result) {
      logger_->warn("Catch-Up-Response was not sent: {}",
                    result.error().message());
    }
  }

  void VotingRoundImpl::pending() {
    pending_timer_.cancel();

    if (stage_ == Stage::COMPLETED) {
      return;
    }

    if (isPrimary_ && primary_vote_.has_value()) {
      sendProposal(primary_vote_.value());
    }
    if (prevote_.has_value()) {
      sendPrevote(prevote_.value());
    }
    if (precommit_.has_value()) {
      sendPrecommit(precommit_.value());
    }

    pending_timer_.expires_from_now(std::max<Clock::Duration>(
        duration_ * 10,
        std::chrono::seconds(10)));  // Should be longer than precommit timer
    pending_timer_.async_wait([this](const auto &...) {
      logger_->debug("Pending in round: {}", round_number_);
      pending();
    });
  }
}  // namespace kagome::consensus::grandpa
