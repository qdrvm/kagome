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

  static auto convertToPrevote = [](const auto &vote) {
    return Prevote(vote.block_number, vote.block_hash);
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
      std::shared_ptr<boost::asio::io_context> io_context,
      const std::shared_ptr<const VotingRound> &previous_round,
      std::shared_ptr<const RoundState> previous_round_state)
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

    if (previous_round) {
      previous_round_state_ = previous_round->state();
    } else {
      previous_round_state_ = std::move(previous_round_state);
    }
    BOOST_ASSERT(previous_round_state_ != nullptr);

    current_round_state_ = std::make_shared<RoundState>(RoundState{
        .last_finalized_block = previous_round_state_->finalized.value_or(
            previous_round_state_->last_finalized_block),
        .best_prevote_candidate =
            convertToPrevote(previous_round_state_->finalized.value_or(
                previous_round_state_->last_finalized_block)),
        .best_final_candidate =
            convertToBlockInfo(previous_round_state_->finalized.value_or(
                previous_round_state_->last_finalized_block))});

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

  void VotingRoundImpl::play() {
    if (stage_ != Stage::INIT) {
      return;
    }
    stage_ = Stage::START;

    logger_->debug("Start round #{}", round_number_);

    // Current local time (Tstart)
    start_time_ = clock_->now();

    // Derive-Primary
    // see ctor

    if (isPrimary_) {
      logger_->debug("Node is primary proposer at round #{}", round_number_);

      // Broadcast Fin-message with previous round best final candidate
      //  (or last finalized otherwise)
      // spec: Broadcast(M vr ¡1;Fin (Best-Final-Candidate(r-1)))

      // if Best-Final-Candidate greater than Last-Finalized-Block
      // spec: if Best-Final-Candidate(r ¡ 1) > Last-Finalized-Block
      if (previous_round_state_->best_final_candidate.block_number
          > previous_round_state_->last_finalized_block.block_number) {
        doProposal();
      }
    }

    startPrevoteStage();
  }

  void VotingRoundImpl::startPrevoteStage() {
    BOOST_ASSERT(stage_ == Stage::START);
    stage_ = Stage::START_PREVOTE;

    logger_->debug("Start prevote stage of round #{}", round_number_);

    // Continue to reveive messages until T>=Tstart + 2 * Duration or round is
    //  completable
    // spec: Receive-Messages(until Time >t r ; v + 2  T or r is
    //  completable)

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
    BOOST_ASSERT(stage_ == Stage::END_PREVOTE);
    stage_ = Stage::START_PRECOMMIT;

    logger_->debug("Start precommit stage of round #{}", round_number_);

    // Continue to reveive messages until T>=Tstart + T * Duration or round is
    // completable
    // spec: Receive-Messages(until B vr;pv > L and (Time >t r ; v + 4  T or r
    // is completable))

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
    BOOST_ASSERT(stage_ == Stage::PRECOMMIT_RUNS);
    stage_ = Stage::END_PRECOMMIT;

    // Broadcast vote for precommit stage
    // spec: Broadcast(M vr;pc (B vr;pv ))
    doPrecommit();

    logger_->debug("End precommit stage of round #{}", round_number_);

    timer_.cancel();
    on_complete_handler_ = nullptr;

    // Trying to finalize round
    // spec: Attempt-To-Finalize-Round(r)

    startWaitingStage();
  }

  void VotingRoundImpl::startWaitingStage() {
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
    // TODO set handler of previous round finalizable

    stage_ = Stage::WAITING_RUNS;
  }

  void VotingRoundImpl::endWaitingStage() {
    BOOST_ASSERT(stage_ == Stage::WAITING_RUNS);
    stage_ = Stage::END_WAITING;

    logger_->debug("End final stage of round #{}", round_number_);

    // reset handler of previous round finalizable
    on_complete_handler_ = nullptr;

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
  }

  void VotingRoundImpl::doProposal() {
    // Send primary propose
    // @spec Broadcast(M vr ¡1;Prim (Best-Final-Candidate(r-1)))

    logger_->debug("Sending primary block hint for round #{}", round_number_);
    primary_vote_ =
        convertToPrimaryPropose(previous_round_state_->best_final_candidate);

    auto signed_primary_proposal =
        vote_crypto_provider_->signPrimaryPropose(primary_vote_.value());

    logger_->debug("Propose block #{} hash={} in round #{}",
                   signed_primary_proposal.block_number(),
                   signed_primary_proposal.block_hash(),
                   round_number_);

    auto proposed = env_->onProposed(
        round_number_, voter_set_->id(), signed_primary_proposal);

    if (not proposed) {
      logger_->error("Primary proposal was not sent in round #{}",
                     proposed.error().message());
      // TODO Do need to handle?
    }
  }

  void VotingRoundImpl::doPrevote() {
    // TODO: make by spec
    //
    //    // spec: L Best-Final-Candidate(r-1)
    //    const auto &best_final_candidate =
    //        previous_round_state_->best_final_candidate;
    //
    //    // spec: N Best-PreVote-Candidate(r)
    //    const auto &best_prevote_candidate =
    //        current_round_state_->best_prevote_candidate;

    // Broadcast vote for prevote stage
    // spec: Broadcast(M vr;pv (N ))

    auto prevote = constructPrevote(*previous_round_state_);
    if (prevote) {
      logger_->debug("Prevote for block #{} hash={} in round #{}",
                     prevote.value().block_number(),
                     prevote.value().block_hash(),
                     round_number_);
      auto prevoted =
          env_->onPrevoted(round_number_, voter_set_->id(), prevote.value());
      if (not prevoted) {
        logger_->error("Prevote was not sent: {}", prevoted.error().message());
      }
    } else {
      logger_->debug("No prevote in round #{}", round_number_);
    }
  }

  void VotingRoundImpl::doPrecommit() {
    auto last_round_estimate = previous_round_state_->best_final_candidate;

    // We should precommit if current state contains prevote and it is
    // either equal to the last round estimate or is descendant of it
    bool should_precommit =
        current_round_state_->best_prevote_candidate.block_hash
            == last_round_estimate.block_hash
        or env_->isEqualOrDescendOf(
            last_round_estimate.block_hash,
            current_round_state_->best_prevote_candidate.block_hash);

    if (should_precommit) {
      logger_->debug("Casting precommit for round {}", round_number_);
      auto precommit = constructPrecommit(*previous_round_state_);
      if (precommit) {
        logger_->debug("Precommit block #{} hash={} in round #{}",
                       precommit.value().block_number(),
                       precommit.value().block_hash(),
                       round_number_);
        auto precommitted = env_->onPrecommitted(
            round_number_, voter_set_->id(), precommit.value());
        if (not precommitted) {
          logger_->error("Precommit was not sent: {}",
                         precommitted.error().message());
          return;
        }
        return;
      } else {
        logger_->debug("No precommit in round #{}", round_number_);
      }
      BOOST_ASSERT_MSG(false, "Not possible. Shouldn't get here");
    }

    env_->onCompleted(VotingRoundError::LAST_ESTIMATE_BETTER_THAN_PREVOTE);
  }

  void VotingRoundImpl::doFinalize() {}

  //  BlockInfo VotingRoundImpl2::getBestFinalCandidate() const {
  //    // 1. Взять ghost
  //    // 2. Взять множество блоков у которых высота меньше либо равна ghost
  //    при
  //    // этом эти блоки могут потенциально набрать больше чем 2/3 голосов
  //    // 3. если таких блоков нет, то
  //    //   4. вернуть ghost
  //    // 5. Из множества C вернуть блок с наибольшей высотой
  //
  //    current_round_state_->prevote_ghost;
  //
  //    previous_round_state_->estimate;
  //
  //    return kagome::consensus::grandpa::BlockInfo();
  //  }
  //
  //  BlockInfo VotingRoundImpl2::getBestPrevoteCandidate() const {
  //    //  	L <- Best-Final-Candidate(r ¡ 1)
  //    //		B vr;pv <- GRANDPA-GHOST(r)
  //    //		if Received(M v r;prim (B)) and B vr;pv > B > L
  //    //			N <- B
  //    //		else
  //    //			N <- B vr;pv
  //
  //    return kagome::consensus::grandpa::BlockInfo();
  //  }
  //
  //  BlockInfo VotingRoundImpl2::getLastFinalizedBlock() const {
  //    return kagome::consensus::grandpa::BlockInfo();
  //  }

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

  void VotingRoundImpl::onFinalize(const Fin &f) {
    logger_->debug("Finalize received for round #{}", round_number_);
    logger_->debug("Received fin message for vote: {}",
                   f.vote.block_hash.toHex());
    // validate message
    if (validate(f.vote, f.justification)) {
      // finalize to state
      auto finalized = env_->finalize(f.vote.block_hash, f.justification);
      if (not finalized) {
        logger_->error(
            "Could not finalize block {} from round {} with error: {}",
            f.vote.block_hash.toHex(),
            f.round_number,
            finalized.error().message());
        return;
      }
      env_->onCompleted(CompletedRound{.round_number = round_number_,
                                       .state = current_round_state_});
    } else {
      logger_->error("Validation of vote {} failed", f.vote.block_hash.toHex());
      env_->onCompleted(VotingRoundError::FIN_VALIDATION_FAILED);
    }
  }

  bool VotingRoundImpl::validate(
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

  bool VotingRoundImpl::tryFinalize() {
    if (not completable()) {
      return false;
    }
    if (not previous_round_state_) {
      logger_->error("Last round state is empty during finalization");
      return false;
    }
    // check if new state differs with the old one and broadcast new state
    if (auto notify_res = notify(*previous_round_state_); not notify_res) {
      logger_->debug("Did not notify. Reason: {}",
                     notify_res.error().message());
      // Round is completable but we cannot notify others. Finish the round
      env_->onCompleted(notify_res.error());
      return false;
    }
    return true;
  }

  outcome::result<void> VotingRoundImpl::notify(
      const RoundState &last_round_state) {
    if (completable_) {
      auto finalized = current_round_state_->finalized.value();
      const auto &opt_justification = finalizingPrecommits(finalized);
      if (not opt_justification) {
        logger_->warn("No justification for block  <{}, {}>",
                      finalized.block_number,
                      finalized.block_hash.toHex());
      }

      auto justification = opt_justification.value();

      OUTCOME_TRY(env_->onCommitted(round_number_, finalized, justification));
      return outcome::success();
    }
    return VotingRoundError::NEW_STATE_EQUAL_TO_OLD;
  }

  RoundNumber VotingRoundImpl::roundNumber() const {
    return round_number_;
  }

  std::shared_ptr<const RoundState> VotingRoundImpl::state() const {
    return current_round_state_;
  }

  void VotingRoundImpl::onPrimaryPropose(const SignedMessage &primary_propose) {
    logger_->debug("Proposal received for round #{}", round_number_);
    if (not isPrimary(primary_propose.id)) {
      logger_->warn("Proposal of {} is not primary",
                    primary_propose.id.toHex());
      return;
    }
    bool isValid = vote_crypto_provider_->verifyPrimaryPropose(primary_propose);
    if (not isValid) {
      logger_->warn("Primary propose of {} has invalid signature",
                    primary_propose.id.toHex());
      return;
    }
    primary_vote_ = PrimaryPropose{primary_propose.block_number(),
                                   primary_propose.block_hash()};
  }

  void VotingRoundImpl::onPrevote(const SignedMessage &prevote) {
    logger_->debug("Prevote received for round #{}", round_number_);

    bool isValid = vote_crypto_provider_->verifyPrevote(prevote);
    if (not isValid) {
      logger_->warn("Prevote of {} has invalid signature", prevote.id.toHex());
      return;
    }
    onSignedPrevote(prevote);
    updatePrevoteGhost();
    update();
    tryFinalize();
  }

  void VotingRoundImpl::onPrecommit(const SignedMessage &precommit) {
    logger_->debug("Precommit received for round #{}", round_number_);

    bool isValid = vote_crypto_provider_->verifyPrecommit(precommit);
    if (not isValid) {
      logger_->warn("Precommit of {} has invalid signature",
                    precommit.id.toHex());
      return;
    }
    if (not onSignedPrecommit(precommit)) {
      env_->onCompleted(VotingRoundError::LAST_ESTIMATE_BETTER_THAN_PREVOTE);
      return;
    }
    update();
    tryFinalize();
  }

  void VotingRoundImpl::onSignedPrevote(const SignedMessage &vote) {
    BOOST_ASSERT(vote.is<Prevote>());
    auto weight = voter_set_->voterWeight(vote.id);
    if (not weight) {
      return;
    }
    switch (prevotes_->push(vote, weight.value())) {
      case VoteTracker::PushResult::SUCCESS: {
        const auto &voters = voter_set_->voters();

        // prepare VoteWeight which contains index of who has voted and what
        // kind of vote it was
        VoteWeight v{voters.size()};
        auto index = voter_set_->voterIndex(vote.id);
        if (not index) {
          logger_->warn("Voter {} is not known: {}", vote.id.toHex());
          return;
        }

        v.prevotes[index.value()] = voter_set_->voterWeight(vote.id).value();

        if (auto inserted = graph_->insert(vote.message, v); not inserted) {
          logger_->warn("Vote {} was not inserted with error: {}",
                        vote.block_hash().toHex(),
                        inserted.error().message());
        }
        break;
      }
      case VoteTracker::PushResult::DUPLICATED: {
        break;
      }
      case VoteTracker::PushResult::EQUIVOCATED: {
        auto index = voter_set_->voterIndex(vote.id);
        if (not index) {
          logger_->warn("Voter {} is not known: {}", vote.id.toHex());
          return;
        }
        prevote_equivocators_[index.value()] = true;
        break;
      }
    }
  }

  bool VotingRoundImpl::onSignedPrecommit(const SignedMessage &vote) {
    BOOST_ASSERT(vote.is<Precommit>());
    auto weight = voter_set_->voterWeight(vote.id);
    if (not weight) {
      return false;
    }
    switch (precommits_->push(vote, weight.value())) {
      case VoteTracker::PushResult::SUCCESS: {
        const auto &voters = voter_set_->voters();

        // prepare VoteWeight which contains index of who has voted and what
        // kind of vote it was
        VoteWeight v{voters.size()};
        auto index = voter_set_->voterIndex(vote.id);
        if (not index) {
          logger_->warn("Voter {} is not known: {}", vote.id.toHex());
          return false;
        }

        v.precommits[index.value()] = voter_set_->voterWeight(vote.id).value();

        if (auto inserted = graph_->insert(vote.message, v); not inserted) {
          logger_->warn("Vote {} was not inserted with error: {}",
                        vote.block_hash().toHex(),
                        inserted.error().message());
          return false;
        }
        break;
      }
      case VoteTracker::PushResult::DUPLICATED: {
        return false;
      }
      case VoteTracker::PushResult::EQUIVOCATED: {
        auto index = voter_set_->voterIndex(vote.id);
        if (not index) {
          logger_->warn("Voter {} is not known: {}", vote.id.toHex());
          return false;
        }
        precommit_equivocators_[index.value()] = true;
        break;
      }
    }
    return true;
  }

  void VotingRoundImpl::updatePrevoteGhost() {
    if (prevotes_->getTotalWeight() >= threshold_) {
      const auto &ghost_block_info =
          convertToBlockInfo(current_round_state_->best_prevote_candidate);

      auto prevote_ghost = graph_->findGhost(
          ghost_block_info, [this](const VoteWeight &vote_weight) {
            return vote_weight
                       .totalWeight(prevote_equivocators_,
                                    precommit_equivocators_,
                                    voter_set_)
                       .prevote
                   >= threshold_;
          });

      if (prevote_ghost.has_value()) {
        current_round_state_->best_prevote_candidate =
            convertToPrevote(prevote_ghost.value());
      }
    }
  }

  bool VotingRoundImpl::completable() const {
    return completable_;
  }

  outcome::result<SignedMessage> VotingRoundImpl::constructPrevote(
      const RoundState &last_round_state) const {
    const auto &last_round_estimate = last_round_state.best_final_candidate;

    // find the block that we should take descendent from
    auto find_descendent_of =
        primary_vote_.map(convertToBlockInfo)
            .map([&](const BlockInfo &primary) {
              // if we have primary_vote then:
              // if last prevote is the same as primary vote, then return it
              // else if primary vote is better than last prevote return last
              // round estimate
              auto last_prevote_g = last_round_state.best_prevote_candidate;

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

    return vote_crypto_provider_->signPrevote(
        {best_chain.block_number, best_chain.block_hash});
  }

  outcome::result<SignedMessage> VotingRoundImpl::constructPrecommit(
      const RoundState &last_round_state) const {
    const auto &target = current_round_state_->best_prevote_candidate;

    return vote_crypto_provider_->signPrecommit({
        target.block_number,
        target.block_hash,
    });
  }

  void VotingRoundImpl::update() {
    // No supermajority in prevotes
    if (prevotes_->getTotalWeight() < threshold_) {
      return;
    }

    const auto &prevote_ghost = current_round_state_->best_prevote_candidate;

    // anything new finalized? finalized blocks are those which have both
    // 2/3+ precommit weight.
    auto current_precommits = precommits_->getTotalWeight();

    if (current_precommits >= threshold_) {
      current_round_state_->finalized = graph_->findAncestor(
          BlockInfo{
              prevote_ghost.block_number,
              prevote_ghost.block_hash,
          },
          [this](const VoteWeight &vote_weight) {
            return vote_weight
                       .totalWeight(prevote_equivocators_,
                                    precommit_equivocators_,
                                    voter_set_)
                       .precommit
                   >= threshold_;
          });
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
      auto block = graph_->findAncestor(
          BlockInfo{prevote_ghost.block_number, prevote_ghost.block_hash},
          possible_to_precommit);
      if (block.has_value()) {
        current_round_state_->best_final_candidate = block.value();
      }
    } else {
      current_round_state_->best_final_candidate =
          BlockInfo{prevote_ghost.block_number, prevote_ghost.block_hash};
      return;
    }

    const auto &block = current_round_state_->best_final_candidate;
    completable_ =

        block.block_hash != prevote_ghost.block_hash
        || graph_->findGhost(block, possible_to_precommit)
               .map([&prevote_ghost](const BlockInfo &block_info) {
                 return std::tie(block_info.block_hash, block_info.block_number)
                        == std::tie(prevote_ghost.block_hash,
                                    prevote_ghost.block_number);
               })
               .value_or(true);
  }

  boost::optional<GrandpaJustification> VotingRoundImpl::finalizingPrecommits(
      const BlockInfo &estimate) const {
    const auto &precommits = precommits_->getMessages();
    GrandpaJustification justification = std::accumulate(
        precommits.begin(),
        precommits.end(),
        GrandpaJustification{},
        [this](GrandpaJustification &j, const auto &precommit_variant) {
          visit_in_place(
              precommit_variant,
              [&j, this](const SignedMessage &voting_message) {
                if (voting_message.is<Precommit>()
                    and env_->isEqualOrDescendOf(
                        current_round_state_->finalized->block_hash,
                        voting_message.block_hash())) {
                  j.items.push_back(voting_message);
                }
              },
              [&](const VoteTracker::EquivocatoryVotingMessage
                      &equivocatory_voting_message) {
                j.items.push_back(equivocatory_voting_message.first);
                j.items.push_back(equivocatory_voting_message.second);
              });
          return j;
        });
    return justification;
  }

}  // namespace kagome::consensus::grandpa
