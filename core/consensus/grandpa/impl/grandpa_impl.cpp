/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/grandpa_impl.hpp"

#include "consensus/grandpa/grandpa_context.hpp"
#include "consensus/grandpa/impl/vote_crypto_provider_impl.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/impl/voting_round_error.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "scale/scale.hpp"

namespace {
  constexpr auto highestGrandpaRoundMetricName =
      "kagome_finality_grandpa_round";
}

namespace kagome::consensus::grandpa {

  GrandpaImpl::GrandpaImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<Environment> environment,
      std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      const std::shared_ptr<crypto::Ed25519Keypair> &keypair,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      std::shared_ptr<authority::AuthorityManager> authority_manager,
      std::shared_ptr<network::Synchronizer> synchronizer,
      std::shared_ptr<network::PeerManager> peer_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree)
      : environment_{std::move(environment)},
        crypto_provider_{std::move(crypto_provider)},
        grandpa_api_{std::move(grandpa_api)},
        keypair_{keypair},
        clock_{std::move(clock)},
        scheduler_{std::move(scheduler)},
        authority_manager_(std::move(authority_manager)),
        synchronizer_(std::move(synchronizer)),
        peer_manager_(std::move(peer_manager)),
        block_tree_(std::move(block_tree)) {
    BOOST_ASSERT(environment_ != nullptr);
    BOOST_ASSERT(crypto_provider_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(clock_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(authority_manager_ != nullptr);
    BOOST_ASSERT(synchronizer_ != nullptr);
    BOOST_ASSERT(peer_manager_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);

    BOOST_ASSERT(app_state_manager != nullptr);

    // Register metrics
    metrics_registry_->registerGaugeFamily(highestGrandpaRoundMetricName,
                                           "Highest GRANDPA round");
    metric_highest_round_ =
        metrics_registry_->registerGaugeMetric(highestGrandpaRoundMetricName);
    metric_highest_round_->set(0);

    app_state_manager->takeControl(*this);
  }

  bool GrandpaImpl::prepare() {
    // Set themselves in environment
    environment_->setJustificationObserver(shared_from_this());
    return true;
  }

  bool GrandpaImpl::start() {
    // Obtain last completed round
    auto round_state_res = getLastCompletedRound();
    if (not round_state_res.has_value()) {
      logger_->critical(
          "Can't retrieve last round data: {}. Stopping grandpa execution",
          round_state_res.error().message());
      return false;
    }
    auto &round_state = round_state_res.value();

    SL_DEBUG(logger_,
             "Grandpa will be started with round #{}",
             round_state.round_number + 1);

    auto authorities_res =
        authority_manager_->authorities(round_state.last_finalized_block, true);
    if (not authorities_res.has_value()) {
      logger_->critical(
          "Can't retrieve authorities for block {}. Stopping grandpa execution",
          round_state.last_finalized_block);
      return false;
    }
    auto &authorities = authorities_res.value();

    auto voters = std::make_shared<VoterSet>(authorities->id);
    for (const auto &authority : *authorities) {
      auto res = voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                                authority.weight);
      if (res.has_error()) {
        logger_->critical(
            "Can't make voter set: {}. Stopping grandpa execution",
            res.error().message());
        return false;
      }
    }

    current_round_ = makeInitialRound(round_state, std::move(voters));
    BOOST_ASSERT_MSG(current_round_ != nullptr,
                     "Initial round must create successful in any case");

    GrandpaImpl::executeNextRound(current_round_->roundNumber());

    if (not current_round_) {
      return false;
    }

    return true;
  }

  void GrandpaImpl::stop() {}

  std::shared_ptr<VotingRound> GrandpaImpl::makeInitialRound(
      const MovableRoundState &round_state, std::shared_ptr<VoterSet> voters) {
    auto vote_graph = std::make_shared<VoteGraphImpl>(
        round_state.last_finalized_block, voters, environment_);

    GrandpaConfig config{.voters = std::move(voters),
                         .round_number = round_state.round_number,
                         .duration = round_time_factor_,
                         .id = keypair_
                                   ? std::make_optional(keypair_->public_key)
                                   : std::nullopt};

    auto vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair_, crypto_provider_, round_state.round_number, config.voters);

    auto new_round = std::make_shared<VotingRoundImpl>(
        shared_from_this(),
        std::move(config),
        authority_manager_,
        environment_,
        std::move(vote_crypto_provider),
        std::make_shared<VoteTrackerImpl>(),  // Prevote tracker
        std::make_shared<VoteTrackerImpl>(),  // Precommit tracker
        std::move(vote_graph),
        clock_,
        scheduler_,
        round_state);

    new_round->end();
    return new_round;
  }

  std::shared_ptr<VotingRound> GrandpaImpl::makeNextRound(
      const std::shared_ptr<VotingRound> &round) {
    BOOST_ASSERT(round->finalizedBlock().has_value());

    BlockInfo best_block = round->finalizedBlock().value();

    auto authorities_opt = authority_manager_->authorities(best_block, true);
    if (!authorities_opt) {
      SL_CRITICAL(logger_,
                  "Can't retrieve authorities for finalized block {}",
                  best_block);
      std::abort();
    }

    auto &authorities = authorities_opt.value();
    BOOST_ASSERT(not authorities->empty());

    auto voters = std::make_shared<VoterSet>(authorities->id);
    for (const auto &authority : *authorities) {
      auto res = voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                                authority.weight);
      if (res.has_error()) {
        SL_CRITICAL(logger_, "Can't make voter set: {}", res.error().message());
        std::abort();
      }
    }

    const auto new_round_number =
        round->voterSetId() == voters->id() ? (round->roundNumber() + 1) : 1;

    auto vote_graph =
        std::make_shared<VoteGraphImpl>(best_block, voters, environment_);

    GrandpaConfig config{.voters = std::move(voters),
                         .round_number = new_round_number,
                         .duration = round_time_factor_,
                         .id = keypair_
                                   ? std::make_optional(keypair_->public_key)
                                   : std::nullopt};

    auto vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair_, crypto_provider_, new_round_number, config.voters);

    auto new_round = std::make_shared<VotingRoundImpl>(
        shared_from_this(),
        std::move(config),
        authority_manager_,
        environment_,
        std::move(vote_crypto_provider),
        std::make_shared<VoteTrackerImpl>(),  // Prevote tracker
        std::make_shared<VoteTrackerImpl>(),  // Precommit tracker
        std::move(vote_graph),
        clock_,
        scheduler_,
        round);
    return new_round;
  }

  std::shared_ptr<VotingRound> GrandpaImpl::selectRound(
      RoundNumber round_number, std::optional<MembershipCounter> voter_set_id) {
    std::shared_ptr<VotingRound> round = current_round_;

    while (round != nullptr) {
      if (round->roundNumber() == round_number) {
        if (not voter_set_id.has_value()
            or round->voterSetId() == voter_set_id.value()) {
          break;
        }
      }
      round = round->getPreviousRound();
    }

    return round;
  }

  outcome::result<MovableRoundState> GrandpaImpl::getLastCompletedRound()
      const {
    auto finalized_block = block_tree_->getLastFinalized();

    if (finalized_block.number == 0) {
      return MovableRoundState{.round_number = 0,
                               .last_finalized_block = finalized_block,
                               .votes = {},
                               .finalized = {finalized_block}};
    }

    OUTCOME_TRY(encoded_justification,
                block_tree_->getBlockJustification(finalized_block.hash));

    OUTCOME_TRY(
        grandpa_justification,
        scale::decode<GrandpaJustification>(encoded_justification.data));

    MovableRoundState round_state{
        .round_number = grandpa_justification.round_number,
        .last_finalized_block = grandpa_justification.block_info,
        .votes = {},
        .finalized = {grandpa_justification.block_info}};

    std::transform(std::move_iterator(grandpa_justification.items.begin()),
                   std::move_iterator(grandpa_justification.items.end()),
                   std::back_inserter(round_state.votes),
                   [](auto &&item) { return std::forward<VoteVariant>(item); });

    return round_state;
  }

  void GrandpaImpl::executeNextRound(RoundNumber round_number) {
    if (current_round_->roundNumber() != round_number) {
      return;
    }

    current_round_ = makeNextRound(current_round_);

    // Truncate chain of rounds
    size_t i = 0;
    for (auto round = current_round_; round != nullptr;
         round = round->getPreviousRound()) {
      if (++i >= kKeepRecentRounds) {
        round->forgetPreviousRound();
      }
    }

    metric_highest_round_->set(current_round_->roundNumber());
    if (keypair_) {
      current_round_->play();
    }
  }

  void GrandpaImpl::updateNextRound(RoundNumber round_number) {
    if (auto round = selectRound(round_number + 1, std::nullopt)) {
      round->update(VotingRound::IsPreviousRoundChanged{true},
                    VotingRound::IsPrevotesChanged{false},
                    VotingRound::IsPrecommitsChanged{false});
    }
  }

  void GrandpaImpl::onNeighborMessage(
      const libp2p::peer::PeerId &peer_id,
      const network::GrandpaNeighborMessage &msg) {
    SL_DEBUG(logger_,
             "NeighborMessage set_id={} round={} last_finalized={} "
             "has received from {}",
             msg.voter_set_id,
             msg.round_number,
             msg.last_finalized,
             peer_id);

    // Ignore peer whose voter_set is different
    if (msg.voter_set_id != current_round_->voterSetId()) {
      return;
    }

    // Check if needed to catch-up peer, then do that
    if (msg.round_number >= current_round_->roundNumber() + kCatchUpThreshold) {
      std::ignore = environment_->onCatchUpRequested(
          peer_id, msg.voter_set_id, msg.round_number - 1);
      return;
    }

    // Iff peer just reached one of recent round, then share known votes
    auto info = peer_manager_->getPeerState(peer_id);
    if (not info.has_value() || msg.voter_set_id != info->set_id
        || msg.round_number > info->round_number) {
      if (auto round = selectRound(msg.round_number, msg.voter_set_id)) {
        environment_->sendState(peer_id, round->state(), msg.voter_set_id);
      }
    }
  }

  void GrandpaImpl::onCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                                     const network::CatchUpRequest &msg) {
    // It is also impolite to send a catch-up request to a peer in a new
    // different Set ID.
    if (msg.voter_set_id != current_round_->voterSetId()) {
      SL_DEBUG(logger_,
               "Catch-up request to round #{} received from {} was rejected: "
               "impolite, because voter set id are differ (our: {}, their: {})",
               msg.round_number,
               peer_id,
               current_round_->voterSetId(),
               msg.voter_set_id);
      return;
    }

    // It is impolite to send a catch-up request for a round `R` to a peer whose
    // announced view is behind `R`.
    if (msg.round_number > current_round_->roundNumber()) {
      SL_DEBUG(logger_,
               "Catch-up request to round #{} received from {} was rejected: "
               "impolite, because our current round is less - {}",
               msg.round_number,
               peer_id,
               current_round_->roundNumber());
      return;
    }

    auto round = selectRound(msg.round_number, msg.voter_set_id);
    if (round == nullptr) {
      SL_DEBUG(logger_,
               "Catch-up request to round #{} received from {} was rejected: "
               "target round not found",
               msg.round_number,
               peer_id);
      return;
    }

    if (not round->finalizedBlock().has_value()) {
      SL_DEBUG(logger_,
               "Catch-up request to round #{} received from {} was rejected: "
               "round is not finalizable",
               msg.round_number,
               peer_id);
      throw std::runtime_error("Need not ensure if it is correct");
      return;
    }

    SL_DEBUG(logger_,
             "Catch-up request to round #{} received from {}",
             msg.round_number,
             peer_id);
    round->doCatchUpResponse(peer_id);
  }

  void GrandpaImpl::onCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                                      const network::CatchUpResponse &msg) {
    BOOST_ASSERT(current_round_ != nullptr);
    // Ignore message of peer whose round in different voter set
    if (msg.voter_set_id != current_round_->voterSetId()) {
      SL_DEBUG(
          logger_,
          "Catch-up response (till round #{}) received from {} was rejected: "
          "impolite, because voter set id are differ (our: {}, their: {})",
          msg.round_number,
          peer_id,
          current_round_->voterSetId(),
          msg.voter_set_id);
      return;
    }

    if (msg.round_number < current_round_->roundNumber()) {
      // Catching up in to the past
      SL_DEBUG(
          logger_,
          "Catch-up response (till round #{}) received from {} was rejected: "
          "catching up into the past",
          msg.round_number,
          peer_id);
      return;
    }

    SL_DEBUG(logger_,
             "Catch-up response (till round #{}) received from {}",
             msg.round_number,
             peer_id);

    GrandpaContext::Guard cg;

    if (msg.round_number > current_round_->roundNumber()) {
      MovableRoundState round_state{
          .round_number = msg.round_number,
          .last_finalized_block = current_round_->lastFinalizedBlock(),
          .votes = {},
          .finalized = msg.best_final_candidate};

      std::transform(msg.prevote_justification.begin(),
                     msg.prevote_justification.end(),
                     std::back_inserter(round_state.votes),
                     [](auto &item) { return item; });
      std::transform(msg.precommit_justification.begin(),
                     msg.precommit_justification.end(),
                     std::back_inserter(round_state.votes),
                     [](auto &item) { return item; });

      auto authorities_opt =
          authority_manager_->authorities(round_state.finalized.value(), false);
      if (!authorities_opt) {
        SL_WARN(logger_,
                "Can't retrieve authorities for finalized block {}",
                round_state.finalized.value());
        return;
      }
      auto &authorities = authorities_opt.value();

      auto voters = std::make_shared<VoterSet>(msg.voter_set_id);
      for (const auto &authority : *authorities) {
        auto res = voters->insert(
            primitives::GrandpaSessionKey(authority.id.id), authority.weight);
        if (res.has_error()) {
          SL_WARN(logger_, "Can't make voter set: {}", res.error().message());
          return;
        }
      }

      auto round = makeInitialRound(round_state, std::move(voters));

      if (not round->completable()) {
        auto ctx = GrandpaContext::get().value();
        // Check if missed block are detected and if this is first attempt
        // (considering by definition peer id in context)
        if (not ctx->missing_blocks.empty() and not ctx->peer_id.has_value()) {
          ctx->peer_id.emplace(peer_id);
          ctx->catch_up_response.emplace(msg);
          loadMissingBlocks();
        }
        return;
      }

      current_round_->end();
      current_round_ = std::move(round);

    } else {
      bool is_prevotes_changed = false;
      bool is_precommits_changed = false;
      for (auto &vote : msg.prevote_justification) {
        if (current_round_->onPrevote(vote,
                                      VotingRound::Propagation::NEEDLESS)) {
          is_prevotes_changed = true;
        }
      }
      for (auto &vote : msg.precommit_justification) {
        if (current_round_->onPrecommit(vote,
                                        VotingRound::Propagation::NEEDLESS)) {
          is_precommits_changed = true;
        }
      }
      if (is_prevotes_changed or is_precommits_changed) {
        current_round_->update(
            VotingRound::IsPreviousRoundChanged{false},
            VotingRound::IsPrevotesChanged{is_prevotes_changed},
            VotingRound::IsPrecommitsChanged{is_precommits_changed});
      }

      SL_DEBUG(logger_, "Catch-up response applied");

      // Check if catch-up round is not completable
      if (not current_round_->completable()) {
        auto ctx = GrandpaContext::get().value();
        // Check if missed block are detected and if this is first attempt
        // (considering by definition peer id in context)
        if (not ctx->missing_blocks.empty() and not ctx->peer_id.has_value()) {
          ctx->peer_id.emplace(peer_id);
          ctx->catch_up_response.emplace(msg);
          loadMissingBlocks();
        }
        return;
      }
    }

    executeNextRound(current_round_->roundNumber());
  }

  void GrandpaImpl::onVoteMessage(const libp2p::peer::PeerId &peer_id,
                                  const VoteMessage &msg) {
    // If a peer is at a given voter set, it is impolite to send messages from
    // an earlier voter set.
    if (msg.counter < current_round_->voterSetId()) {
      SL_DEBUG(
          logger_,
          "{} signed by {} with set_id={} in round={} has received from {} "
          "and rejected as impolite (our set id is {})",
          msg.vote.is<Prevote>()     ? "Prevote"
          : msg.vote.is<Precommit>() ? "Precommit"
                                     : "PrimaryPropose",
          msg.id(),
          msg.counter,
          msg.round_number,
          peer_id,
          current_round_->voterSetId());
      return;
    }

    // It is extremely impolite to send messages from a future voter set.
    // "future-set" messages can be dropped and ignored.
    if (msg.counter > current_round_->voterSetId()) {
      SL_WARN(logger_,
              "{} signed by {} with set_id={} in round={} has received from {} "
              "and rejected as extremely impolite (our set id is {})",
              msg.vote.is<Prevote>()     ? "Prevote"
              : msg.vote.is<Precommit>() ? "Precommit"
                                         : "PrimaryPropose",
              msg.id(),
              msg.counter,
              msg.round_number,
              peer_id,
              current_round_->voterSetId());
      return;
    }

    // If a peer is at round r, is impolite to send messages about r-2 or
    // earlier
    if (msg.round_number + 2 < current_round_->roundNumber()) {
      SL_DEBUG(
          logger_,
          "{} signed by {} with set_id={} in round={} has received from {} "
          "and rejected as impolite (our round is {})",
          msg.vote.is<Prevote>()     ? "Prevote"
          : msg.vote.is<Precommit>() ? "Precommit"
                                     : "PrimaryPropose",
          msg.id(),
          msg.counter,
          msg.round_number,
          peer_id,
          current_round_->roundNumber());
      return;
    }

    // If a peer is at round r, is extremely impolite to send messages about r+1
    // or later. "future-round" messages can be dropped and ignored.
    if (msg.round_number >= current_round_->roundNumber() + 1) {
      SL_WARN(logger_,
              "{} signed by {} with set_id={} in round={} has received from {} "
              "and rejected as extremely impolite (our round is {})",
              msg.vote.is<Prevote>()     ? "Prevote"
              : msg.vote.is<Precommit>() ? "Precommit"
                                         : "PrimaryPropose",
              msg.id(),
              msg.counter,
              msg.round_number,
              peer_id,
              current_round_->roundNumber());
      return;
    }

    std::shared_ptr<VotingRound> target_round =
        selectRound(msg.round_number, msg.counter);
    if (not target_round) {
      SL_DEBUG(
          logger_,
          "{} signed by {} with set_id={} in round={} has received from {} "
          "and rejected (round not found)",
          msg.vote.is<Prevote>()     ? "Prevote"
          : msg.vote.is<Precommit>() ? "Precommit"
                                     : "PrimaryPropose",
          msg.id(),
          msg.counter,
          msg.round_number,
          peer_id);
      return;
    }

    SL_DEBUG(logger_,
             "{} signed by {} with set_id={} in round={} for block {} "
             "has received from {}",
             msg.vote.is<Prevote>()     ? "Prevote"
             : msg.vote.is<Precommit>() ? "Precommit"
                                        : "PrimaryPropose",
             msg.id(),
             msg.counter,
             msg.round_number,
             msg.vote.getBlockInfo(),
             peer_id);

    GrandpaContext::Guard cg;

    bool is_prevotes_changed = false;
    bool is_precommits_changed = false;
    visit_in_place(
        msg.vote.message,
        [&](const PrimaryPropose &) {
          target_round->onProposal(msg.vote,
                                   VotingRound::Propagation::REQUESTED);
        },
        [&](const Prevote &) {
          if (target_round->onPrevote(msg.vote,
                                      VotingRound::Propagation::REQUESTED)) {
            is_prevotes_changed = true;
          }
        },
        [&](const Precommit &) {
          if (target_round->onPrecommit(msg.vote,
                                        VotingRound::Propagation::REQUESTED)) {
            is_precommits_changed = true;
          }
        });
    if (is_prevotes_changed or is_precommits_changed) {
      target_round->update(
          VotingRound::IsPreviousRoundChanged{false},
          VotingRound::IsPrevotesChanged{is_prevotes_changed},
          VotingRound::IsPrecommitsChanged{is_precommits_changed});
    }

    if (not target_round->finalizedBlock().has_value()) {
      auto ctx = GrandpaContext::get().value();
      // Check if missed block are detected and if this is first attempt
      // (considering by definition peer id in context)
      if (not ctx->missing_blocks.empty() and not ctx->peer_id.has_value()) {
        ctx->peer_id.emplace(peer_id);
        ctx->vote.emplace(msg);
        loadMissingBlocks();
      }
      return;
    }
  }

  void GrandpaImpl::onCommitMessage(const libp2p::peer::PeerId &peer_id,
                                    const network::FullCommitMessage &msg) {
    // It is especially impolite to send commits which are invalid, or from
    // a different Set ID than the receiving peer has indicated
    if (msg.set_id != current_round_->voterSetId()) {
      SL_DEBUG(
          logger_,
          "Commit with set_id={} in round={} for block {} has received from {} "
          "and dropped as impolite: our voter set id is {}",
          msg.set_id,
          msg.round,
          BlockInfo(msg.message.target_number, msg.message.target_hash),
          peer_id,
          current_round_->voterSetId());
      return;
    }

    // It is impolite to send commits which are earlier than the last commit
    // sent
    if (msg.round + kKeepRecentRounds < current_round_->voterSetId()) {
      SL_DEBUG(
          logger_,
          "Commit with set_id={} in round={} for block {} has received from {} "
          "and dropped as impolite: too old commit, our round is {}",
          msg.set_id,
          msg.round,
          BlockInfo(msg.message.target_number, msg.message.target_hash),
          peer_id,
          current_round_->roundNumber());
      return;
    }

    if (msg.round < current_round_->roundNumber()) {
      SL_DEBUG(
          logger_,
          "Commit with set_id={} in round={} for block {} has received from {} "
          "and dropped as fulfilled",
          msg.set_id,
          msg.round,
          BlockInfo(msg.message.target_number, msg.message.target_hash),
          peer_id);
      return;
    }

    SL_DEBUG(logger_,
             "Commit with set_id={} in round={} for block {} "
             "has received from {}",
             msg.set_id,
             msg.round,
             BlockInfo(msg.message.target_number, msg.message.target_hash),
             peer_id);

    GrandpaJustification justification{
        .round_number = msg.round,
        .block_info =
            BlockInfo(msg.message.target_number, msg.message.target_hash)};
    for (size_t i = 0; i < msg.message.precommits.size(); ++i) {
      SignedPrecommit commit;
      commit.message = msg.message.precommits[i];
      commit.signature = msg.message.auth_data[i].first;
      commit.id = msg.message.auth_data[i].second;
      justification.items.emplace_back(std::move(commit));
    }

    GrandpaContext::Guard cg;
    auto ctx = GrandpaContext::get().value();
    ctx->peer_id.emplace(peer_id);
    ctx->commit.emplace(msg);

    auto res = applyJustification(justification.block_info, justification);
    if (not res.has_value()) {
      SL_WARN(
          logger_,
          "Commit with set_id={} in round={} for block {} has received from {} "
          "and has not applied: {}",
          msg.set_id,
          msg.round,
          BlockInfo(msg.message.target_number, msg.message.target_hash),
          peer_id,
          res.error().message());
      return;
    }
  }

  outcome::result<void> GrandpaImpl::applyJustification(
      const BlockInfo &block_info, const GrandpaJustification &justification) {
    auto round = selectRound(justification.round_number, std::nullopt);
    bool need_to_make_round_current = false;
    if (round == nullptr) {
      // This is justification for non-actual round
      if (justification.round_number < current_round_->roundNumber()) {
        return VotingRoundError::JUSTIFICATION_FOR_ROUND_IN_PAST;
      }

      // This is justification for already finalized block
      if (current_round_->lastFinalizedBlock().number > block_info.number) {
        return VotingRoundError::JUSTIFICATION_FOR_BLOCK_IN_PAST;
      }

      MovableRoundState round_state{
          .round_number = justification.round_number,
          .last_finalized_block = current_round_->lastFinalizedBlock(),
          .votes = {},
          .finalized = block_info};

      auto authorities_opt = authority_manager_->authorities(
          round_state.last_finalized_block, true);
      if (!authorities_opt) {
        SL_WARN(logger_,
                "Can't retrieve authorities to apply a justification "
                "at block {}",
                block_info);
        return VotingRoundError::NO_KNOWN_AUTHORITIES_FOR_BLOCK;
      }
      auto &authorities = authorities_opt.value();

      auto voters = std::make_shared<VoterSet>(authorities->id);
      for (const auto &authority : *authorities) {
        auto res = voters->insert(
            primitives::GrandpaSessionKey(authority.id.id), authority.weight);
        if (res.has_error()) {
          SL_CRITICAL(
              logger_, "Can't make voter set: {}", res.error().message());
          return res.as_failure();
        }
      }

      round = makeInitialRound(round_state, std::move(voters));
      need_to_make_round_current = true;
      BOOST_ASSERT(round);

      SL_DEBUG(logger_,
               "Rewind grandpa till round #{} by received justification",
               justification.round_number);
    }

    OUTCOME_TRY(round->applyJustification(block_info, justification));

    if (need_to_make_round_current) {
      current_round_->end();
      current_round_ = std::move(round);
      executeNextRound(current_round_->roundNumber());
    }

    return outcome::success();
  }

  void GrandpaImpl::loadMissingBlocks() {
    auto ctx = GrandpaContext::get().value();
    BOOST_ASSERT(ctx);

    if (not ctx->peer_id.has_value()) {
      return;
    }

    if (ctx->missing_blocks.empty()) {
      return;
    }

    auto final = [wp = weak_from_this(), ctx] {
      if (auto self = wp.lock()) {
        GrandpaContext::set(ctx);
        if (ctx->vote.has_value()) {
          self->onVoteMessage(ctx->peer_id.value(), ctx->vote.value());
        } else if (ctx->catch_up_response.has_value()) {
          self->onCatchUpResponse(ctx->peer_id.value(),
                                  ctx->catch_up_response.value());
        } else if (ctx->commit.has_value()) {
          self->onCommitMessage(ctx->peer_id.value(), ctx->commit.value());
        }
      }
    };

    auto do_request_ptr = std::make_shared<std::function<void()>>();
    auto &do_request = *do_request_ptr;

    do_request = [wp = weak_from_this(),
                  ctx = std::move(ctx),
                  do_request_ptr = std::move(do_request_ptr),
                  final = std::move(final)]() mutable {
      if (auto self = wp.lock()) {
        auto &peer_id = ctx->peer_id.value();
        auto &blocks = ctx->missing_blocks;
        if (not blocks.empty()) {
          auto it = blocks.rbegin();
          auto node = blocks.extract((++it).base());
          auto block = node.value();
          self->synchronizer_->syncByBlockInfo(
              block,
              peer_id,
              [wp, ctx, do_request_ptr = std::move(do_request_ptr)](auto res) {
                if (do_request_ptr != nullptr) {
                  auto do_request = std::move(*do_request_ptr);
                  do_request();
                }
              },
              true);
          return;
        }
        final();
        do_request_ptr.reset();
      }
    };

    do_request();
  }
}  // namespace kagome::consensus::grandpa
