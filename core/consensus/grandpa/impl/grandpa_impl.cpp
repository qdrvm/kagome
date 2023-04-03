/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/grandpa_impl.hpp"

#include "application/app_state_manager.hpp"
#include "application/chain_spec.hpp"
#include "blockchain/block_tree.hpp"
#include "common/tagged.hpp"
#include "consensus/grandpa/authority_manager.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/grandpa_config.hpp"
#include "consensus/grandpa/grandpa_context.hpp"
#include "consensus/grandpa/impl/vote_crypto_provider_impl.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "consensus/grandpa/voting_round_error.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/peer_manager.hpp"
#include "network/reputation_repository.hpp"
#include "network/synchronizer.hpp"

namespace {
  using IsBlockFinalized = kagome::Tagged<bool, struct IsBlockFinalizedTag>;

  constexpr auto highestGrandpaRoundMetricName =
      "kagome_finality_grandpa_round";

  template <typename D>
  auto toMilliseconds(D &&duration) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
  }
}  // namespace

namespace kagome::consensus::grandpa {
  inline bool isWestendPastRound(const primitives::BlockHash &genesis,
                                 const primitives::BlockInfo &block) {
    static auto westend_genesis =
        primitives::BlockHash::fromHex(
            "e143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e")
            .value();
    static primitives::BlockInfo past_round{
        198785,
        primitives::BlockHash::fromHex(
            "62caf6a8c99d63744f7093bceead8fdf4c7d8ef74f16163ed58b1c1aec67bf18")
            .value(),
    };
    return genesis == westend_genesis && block == past_round;
  }

  namespace {
    Clock::Duration getGossipDuration(const application::ChainSpec &chain) {
      // https://github.com/paritytech/polkadot/pull/5448
      auto slow = chain.isVersi() || chain.isWococo() || chain.isRococo()
               || chain.isKusama();
      return std::chrono::duration_cast<Clock::Duration>(
          std::chrono::milliseconds{slow ? 2000 : 1000});
    }
  }  // namespace

  GrandpaImpl::GrandpaImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<Environment> environment,
      std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      std::shared_ptr<crypto::Ed25519Keypair> keypair,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      std::shared_ptr<AuthorityManager> authority_manager,
      std::shared_ptr<network::Synchronizer> synchronizer,
      std::shared_ptr<network::PeerManager> peer_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<network::ReputationRepository> reputation_repository)
      : round_time_factor_{getGossipDuration(chain_spec)},
        environment_{std::move(environment)},
        crypto_provider_{std::move(crypto_provider)},
        grandpa_api_{std::move(grandpa_api)},
        keypair_{std::move(keypair)},
        clock_{std::move(clock)},
        scheduler_{std::move(scheduler)},
        authority_manager_(std::move(authority_manager)),
        synchronizer_(std::move(synchronizer)),
        peer_manager_(std::move(peer_manager)),
        block_tree_(std::move(block_tree)),
        reputation_repository_(std::move(reputation_repository)) {
    BOOST_ASSERT(environment_ != nullptr);
    BOOST_ASSERT(crypto_provider_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(clock_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(authority_manager_ != nullptr);
    BOOST_ASSERT(synchronizer_ != nullptr);
    BOOST_ASSERT(peer_manager_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(reputation_repository_ != nullptr);

    BOOST_ASSERT(app_state_manager != nullptr);

    // Register metrics
    metrics_registry_->registerGaugeFamily(highestGrandpaRoundMetricName,
                                           "Highest GRANDPA round");
    metric_highest_round_ =
        metrics_registry_->registerGaugeMetric(highestGrandpaRoundMetricName);
    metric_highest_round_->set(0);

    // allow app state manager to prepare, start and stop grandpa consensus
    // pipeline
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
          round_state_res.error());
      return false;
    }
    auto &round_state = round_state_res.value();

    SL_DEBUG(logger_,
             "Grandpa will be started with round #{}",
             round_state.round_number + 1);

    auto authorities_res = authority_manager_->authorities(
        round_state.last_finalized_block, IsBlockFinalized{false});
    if (not authorities_res.has_value()) {
      logger_->critical(
          "Can't retrieve authorities for block {}. Stopping grandpa execution",
          round_state.last_finalized_block);
      return false;
    }
    auto &authority_set = authorities_res.value();

    auto voters = std::make_shared<VoterSet>(authority_set->id);
    for (const auto &authority : authority_set->authorities) {
      auto res = voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                                authority.weight);
      if (res.has_error()) {
        logger_->critical(
            "Can't make voter set: {}. Stopping grandpa execution",
            res.error());
        return false;
      }
    }

    current_round_ = makeInitialRound(round_state, std::move(voters));
    BOOST_ASSERT(current_round_ != nullptr);

    if (not current_round_->finalizedBlock().has_value()) {
      logger_->critical(
          "Initial round must be finalized, but it is not. "
          "Stopping grandpa execution");
      return false;
    }

    // Timer to send neighbor message if round does not change long time (1 min)
    fallback_timer_handle_ = scheduler_->scheduleWithHandle(
        [wp = weak_from_this()] {
          auto self = wp.lock();
          if (not self) {
            return;
          }
          BOOST_ASSERT_MSG(self->current_round_,
                           "Current round must be defiled anytime after start");
          auto round =
              std::dynamic_pointer_cast<VotingRoundImpl>(self->current_round_);
          if (round) {
            round->sendNeighborMessage();
          }

          std::ignore =
              self->fallback_timer_handle_.reschedule(std::chrono::minutes(1));
        },
        std::chrono::minutes(1));

    tryExecuteNextRound(current_round_);

    return true;
  }

  void GrandpaImpl::stop() {
    fallback_timer_handle_.cancel();
  }

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

    new_round->end();  // it is okay, because we do not want to actually execute
                       // this round
    return new_round;
  }

  outcome::result<std::shared_ptr<VotingRound>> GrandpaImpl::makeNextRound(
      const std::shared_ptr<VotingRound> &round) {
    BlockInfo best_block =
        round->finalizedBlock().value_or(round->lastFinalizedBlock());

    auto authorities_opt =
        authority_manager_->authorities(best_block, IsBlockFinalized{true});
    if (!authorities_opt) {
      SL_WARN(logger_,
              "Can't retrieve authorities for finalized block {}",
              best_block);
      return VotingRoundError::VOTER_SET_NOT_FOUND_FOR_BLOCK;
    }

    auto &authority_set = authorities_opt.value();
    BOOST_ASSERT(not authority_set->authorities.empty());

    auto voters = std::make_shared<VoterSet>(authority_set->id);
    for (const auto &authority : authority_set->authorities) {
      auto res = voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                                authority.weight);
      if (res.has_error()) {
        SL_WARN(logger_, "Can't make voter set: {}", res.error());
        return res.as_failure();
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

  std::optional<std::shared_ptr<VotingRound>> GrandpaImpl::selectRound(
      RoundNumber round_number, std::optional<VoterSetId> voter_set_id) {
    std::shared_ptr<VotingRound> round = current_round_;

    while (round != nullptr) {
      // Probably came to the round with previous voter set
      if (round->roundNumber() < round_number) {
        return std::nullopt;
      }

      // Round found; check voter set
      if (round->roundNumber() == round_number) {
        if (not voter_set_id.has_value()
            or round->voterSetId() == voter_set_id.value()) {
          break;
        }
      }

      // Go to the previous round
      round = round->getPreviousRound();
    }

    return round == nullptr ? std::nullopt : std::make_optional(round);
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

  void GrandpaImpl::tryExecuteNextRound(
      const std::shared_ptr<VotingRound> &prev_round) {
    if (current_round_ != prev_round) {
      return;
    }

    auto res = makeNextRound(current_round_);
    BOOST_ASSERT_MSG(res.value(),
                     "Next round for current must be created anyway");
    current_round_ = std::move(res.value());

    std::ignore = fallback_timer_handle_.reschedule(std::chrono::minutes(1));

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
    } else {
      auto round = std::dynamic_pointer_cast<VotingRoundImpl>(current_round_);
      if (round) {
        round->sendNeighborMessage();
      }
    }
  }

  void GrandpaImpl::updateNextRound(RoundNumber round_number) {
    if (auto opt_round = selectRound(round_number + 1, std::nullopt);
        opt_round.has_value()) {
      auto &round = opt_round.value();
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

    auto info = peer_manager_->getPeerState(peer_id);

    // Iff peer just reached one of recent round, then share known votes
    if (not info.has_value()
        or (info->get().set_id.has_value()
            and msg.voter_set_id != info->get().set_id)
        or (info->get().round_number.has_value()
            and msg.round_number > info->get().round_number)) {
      if (auto opt_round = selectRound(msg.round_number, msg.voter_set_id);
          opt_round.has_value()) {
        auto &round = opt_round.value();
        environment_->sendState(peer_id, round->state(), msg.voter_set_id);
      }
    }

    bool reputation_changed = false;
    if (info.has_value() and info->get().set_id.has_value()
        and info->get().round_number.has_value()) {
      const auto prev_set_id = info->get().set_id.value();
      const auto prev_round_number = info->get().round_number.value();

      // bad order of set id
      if (msg.voter_set_id < prev_set_id) {
        reputation_repository_->change(
            peer_id, network::reputation::cost::INVALID_VIEW_CHANGE);
        reputation_changed = true;
      }

      // bad order of round number
      if (msg.voter_set_id == prev_set_id
          and msg.round_number < prev_round_number) {
        reputation_repository_->change(
            peer_id, network::reputation::cost::INVALID_VIEW_CHANGE);
        reputation_changed = true;
      }
    }

    peer_manager_->updatePeerState(peer_id, msg);

    if (not reputation_changed) {
      reputation_repository_->change(
          peer_id, network::reputation::benefit::NEIGHBOR_MESSAGE);
    }

    // If peer has the same voter set id
    if (msg.voter_set_id == current_round_->voterSetId()) {
      // Check if needed to catch-up peer, then do that
      if (msg.round_number
          >= current_round_->roundNumber() + kCatchUpThreshold) {
        // Do catch-up only when another one is not in progress
        if (not pending_catchup_request_.has_value()) {
          auto res = environment_->onCatchUpRequested(
              peer_id, msg.voter_set_id, msg.round_number - 1);
          if (res.has_value()) {
            if (pending_catchup_request_.has_value()) {
              SL_WARN(logger_,
                      "Catch up request pending, but another one has done");
            }
            pending_catchup_request_.emplace(
                peer_id,
                network::CatchUpRequest{msg.round_number - 1,
                                        msg.voter_set_id});
            catchup_request_timer_handle_ = scheduler_->scheduleWithHandle(
                [wp = weak_from_this()] {
                  auto self = wp.lock();
                  if (not self) {
                    return;
                  }
                  if (self->pending_catchup_request_.has_value()) {
                    const auto &peer_id =
                        std::get<0>(self->pending_catchup_request_.value());
                    self->reputation_repository_->change(
                        peer_id,
                        network::reputation::cost::CATCH_UP_REQUEST_TIMEOUT);
                    self->pending_catchup_request_.reset();
                  }
                },
                toMilliseconds(kCatchupRequestTimeout));
          }
        }
      }
      return;
    }

    // Ignore peer whose voter set id lower than our current
    if (msg.voter_set_id < current_round_->voterSetId()) {
      return;
    }

    if (info->get().last_finalized <= block_tree_->bestLeaf().number) {
      //  Trying to substitute with justifications' request only
      auto last_finalized = block_tree_->getLastFinalized();
      synchronizer_->syncMissingJustifications(
          peer_id,
          last_finalized,
          std::nullopt,
          [wp = weak_from_this(), last_finalized, msg](auto res) {
            auto self = wp.lock();
            if (not self) {
              return;
            }
            if (res.has_error()) {
              SL_DEBUG(self->logger_,
                      "Missing justifications between blocks {} and "
                      "{} was not loaded: {}",
                      last_finalized,
                      msg.last_finalized,
                      res.error());
            } else {
              SL_DEBUG(self->logger_,
                       "Loaded justifications for blocks in range {} - {}",
                       last_finalized,
                       res.value());
            }
          });
    }
  }

  void GrandpaImpl::onCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                                     const network::CatchUpRequest &msg) {
    auto info_opt = peer_manager_->getPeerState(peer_id);
    if (not info_opt.has_value() or not info_opt->get().set_id.has_value()
        or not info_opt->get().round_number.has_value()) {
      SL_DEBUG(logger_,
               "Catch-up request to round #{} received from {} was rejected: "
               "we are not have our view about remote peer",
               msg.round_number,
               peer_id);
      reputation_repository_->change(
          peer_id, network::reputation::cost::OUT_OF_SCOPE_MESSAGE);
      return;
    }
    const auto &info = info_opt->get();

    // Check if request is corresponding our view about remote peer by set id
    if (msg.voter_set_id != info.set_id.value()) {
      SL_DEBUG(logger_,
               "Catch-up request to round #{} received from {} was rejected: "
               "it is not corresponding our view about remote peer ",
               msg.round_number,
               peer_id,
               current_round_->voterSetId(),
               msg.voter_set_id);

      // NOTE: When we're close to a set change there is potentially a
      // race where the peer sent us the request before it observed that
      // we had transitioned to a new set. In this case we charge a lower
      // cost.
      if (msg.voter_set_id == info.set_id.value()
          and msg.round_number
                  < info.round_number.value() + kCatchUpThreshold) {
        reputation_repository_->change(
            peer_id, network::reputation::cost::HONEST_OUT_OF_SCOPE_CATCH_UP);
        return;
      }

      reputation_repository_->change(
          peer_id, network::reputation::cost::OUT_OF_SCOPE_MESSAGE);
      return;
    }

    // Check if request is corresponding our view about remote peer by round
    // number
    if (msg.round_number <= info.round_number.value()) {
      SL_DEBUG(logger_,
               "Catch-up request to round #{} received from {} was rejected: "
               "it is not corresponding our view about remote peer ",
               msg.round_number,
               peer_id,
               current_round_->voterSetId(),
               msg.voter_set_id);

      reputation_repository_->change(
          peer_id, network::reputation::cost::OUT_OF_SCOPE_MESSAGE);
      return;
    }

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

      reputation_repository_->change(
          peer_id, network::reputation::cost::OUT_OF_SCOPE_MESSAGE);
      return;
    }

    auto opt_round = selectRound(msg.round_number, msg.voter_set_id);
    if (!opt_round.has_value()) {
      SL_DEBUG(logger_,
               "Catch-up request to round #{} received from {} was rejected: "
               "target round not found",
               msg.round_number,
               peer_id);
      return;
    }

    auto &round = opt_round.value();
    if (not round->finalizedBlock().has_value()) {
      SL_DEBUG(logger_,
               "Catch-up request to round #{} received from {} was rejected: "
               "round is not finalizable",
               msg.round_number,
               peer_id);
      throw std::runtime_error("Need not ensure if it is correct");
    }

    SL_DEBUG(logger_,
             "Catch-up request to round #{} received from {}",
             msg.round_number,
             peer_id);
    round->doCatchUpResponse(peer_id);

    reputation_repository_->change(peer_id,
                                   network::reputation::cost::CATCH_UP_REPLY);
  }

  void GrandpaImpl::onCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                                      const network::CatchUpResponse &msg) {
    bool need_cleanup_when_exiting_scope = false;
    GrandpaContext::Guard cg;

    auto ctx = GrandpaContext::get().value();
    if (not ctx->peer_id.has_value()) {
      if (not pending_catchup_request_.has_value()) {
        SL_DEBUG(logger_,
                 "Catch-up request to round #{} received from {}, "
                 "but catch-up request is not pending or timed out",
                 msg.round_number,
                 peer_id);
        reputation_repository_->change(
            peer_id, network::reputation::cost::MALFORMED_CATCH_UP);
        return;
      }

      const auto &[remote_peer_id, catchup_request] =
          pending_catchup_request_.value();

      if (peer_id != remote_peer_id) {
        SL_DEBUG(logger_,
                 "Catch-up request to round #{} received from {}, "
                 "but last catch-up request was sent to {}",
                 msg.round_number,
                 peer_id,
                 remote_peer_id);
        reputation_repository_->change(
            peer_id, network::reputation::cost::OUT_OF_SCOPE_MESSAGE);
        return;
      }

      if (msg.voter_set_id != catchup_request.voter_set_id) {
        SL_DEBUG(logger_,
                 "Catch-up request to round #{} received from {}, "
                 "but last catch-up request was sent for voter set {} "
                 "(received for {})",
                 msg.round_number,
                 peer_id,
                 catchup_request.voter_set_id,
                 msg.voter_set_id);
        reputation_repository_->change(
            peer_id, network::reputation::cost::MALFORMED_CATCH_UP);
        return;
      }

      if (msg.round_number < catchup_request.round_number) {
        SL_DEBUG(logger_,
                 "Catch-up request to round #{} received from {}, "
                 "but last catch-up request was sent for round {}",
                 msg.round_number,
                 peer_id,
                 catchup_request.round_number);
        reputation_repository_->change(
            peer_id, network::reputation::cost::MALFORMED_CATCH_UP);
        return;
      }

      if (msg.prevote_justification.empty()
          or msg.precommit_justification.empty()) {
        SL_DEBUG(logger_,
                 "Catch-up request to round #{} received from {}, "
                 "without any votes",
                 msg.round_number,
                 peer_id);
        reputation_repository_->change(
            peer_id, network::reputation::cost::MALFORMED_CATCH_UP);
        return;
      }

      need_cleanup_when_exiting_scope = true;
    }

    auto cleanup = gsl::finally([&] {
      if (need_cleanup_when_exiting_scope) {
        catchup_request_timer_handle_.cancel();
        pending_catchup_request_.reset();
      }
    });

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

      auto authorities_opt = authority_manager_->authorities(
          round_state.finalized.value(), IsBlockFinalized{false});
      if (!authorities_opt) {
        SL_WARN(logger_,
                "Can't retrieve authorities for finalized block {}",
                round_state.finalized.value());
        return;
      }
      auto &authority_set = authorities_opt.value();

      auto voters = std::make_shared<VoterSet>(msg.voter_set_id);
      for (const auto &authority : authority_set->authorities) {
        auto res = voters->insert(
            primitives::GrandpaSessionKey(authority.id.id), authority.weight);
        if (res.has_error()) {
          SL_WARN(logger_, "Can't make voter set: {}", res.error());
          return;
        }
      }

      auto round = makeInitialRound(round_state, std::move(voters));

      if (not round->completable()
          and not round->finalizedBlock().has_value()) {
        // Met unknown voter - cost reputation
        if (ctx->unknown_voter_counter > 0) {
          reputation_repository_->change(
              peer_id,
              network::reputation::cost::UNKNOWN_VOTER
                  * ctx->unknown_voter_counter);
        }
        // Met invalid signature - cost reputation
        if (ctx->invalid_signature_counter > 0) {
          reputation_repository_->change(
              peer_id,
              network::reputation::cost::BAD_CATCHUP_RESPONSE
                  * ctx->checked_signature_counter);
        }
        // Check if missed block are detected and if this is first attempt
        // (considering by definition peer id in context)
        if (not ctx->missing_blocks.empty()) {
          if (not ctx->peer_id.has_value()) {
            ctx->peer_id.emplace(peer_id);
            ctx->catch_up_response.emplace(msg);
            loadMissingBlocks();
          }
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
        // Met unknown voter - cost reputation
        if (ctx->unknown_voter_counter > 0) {
          reputation_repository_->change(
              peer_id,
              network::reputation::cost::UNKNOWN_VOTER
                  * ctx->unknown_voter_counter);
        }
        // Met invalid signature - cost reputation
        if (ctx->invalid_signature_counter > 0) {
          reputation_repository_->change(
              peer_id,
              network::reputation::cost::BAD_CATCHUP_RESPONSE
                  * ctx->checked_signature_counter);
        }
        // Check if missed block are detected and if this is first attempt
        // (considering by definition peer id in context)
        if (not ctx->missing_blocks.empty()) {
          if (not ctx->peer_id.has_value()) {
            ctx->peer_id.emplace(peer_id);
            ctx->catch_up_response.emplace(msg);
            loadMissingBlocks();
          }
        }
        return;
      }
    }

    tryExecuteNextRound(current_round_);

    reputation_repository_->change(
        peer_id, network::reputation::benefit::BASIC_VALIDATED_CATCH_UP);
  }

  void GrandpaImpl::onVoteMessage(const libp2p::peer::PeerId &peer_id,
                                  const VoteMessage &msg) {
    auto info = peer_manager_->getPeerState(peer_id);
    if (not info.has_value() or not info->get().set_id.has_value()
        or not info->get().round_number.has_value()) {
      SL_DEBUG(
          logger_,
          "{} signed by {} with set_id={} in round={} has received from {} "
          "and we are not have our view about remote peer",
          msg.vote.is<Prevote>()     ? "Prevote"
          : msg.vote.is<Precommit>() ? "Precommit"
                                     : "PrimaryPropose",
          msg.id(),
          msg.counter,
          msg.round_number,
          peer_id,
          current_round_->voterSetId());
      reputation_repository_->change(
          peer_id, network::reputation::cost::OUT_OF_SCOPE_MESSAGE);
      return;
    }

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
      reputation_repository_->change(peer_id,
                                     network::reputation::cost::PAST_REJECTION);
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
      reputation_repository_->change(peer_id,
                                     network::reputation::cost::FUTURE_MESSAGE);
      return;
    }

    if (msg.round_number > current_round_->roundNumber() + 1) {
      reputation_repository_->change(peer_id,
                                     network::reputation::cost::FUTURE_MESSAGE);
    } else if (msg.round_number < current_round_->roundNumber() - 1) {
      reputation_repository_->change(peer_id,
                                     network::reputation::cost::PAST_REJECTION);
    }

    // If the current peer is at round r, it is impolite to receive messages
    // about r-2 or earlier
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

    std::optional<std::shared_ptr<VotingRound>> opt_target_round =
        selectRound(msg.round_number, msg.counter);
    if (not opt_target_round.has_value()) {
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
    auto &target_round = opt_target_round.value();

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

    auto ctx = GrandpaContext::get().value();

    // Met invalid signature - cost reputation
    if (ctx->invalid_signature_counter > 0) {
      reputation_repository_->change(peer_id,
                                     network::reputation::cost::BAD_SIGNATURE
                                         * ctx->checked_signature_counter);
    }

    // Met unknown voter - cost reputation
    if (ctx->unknown_voter_counter > 0) {
      reputation_repository_->change(peer_id,
                                     network::reputation::cost::UNKNOWN_VOTER
                                         * ctx->unknown_voter_counter);
    }

    if (is_prevotes_changed or is_precommits_changed) {
      target_round->update(
          VotingRound::IsPreviousRoundChanged{false},
          VotingRound::IsPrevotesChanged{is_prevotes_changed},
          VotingRound::IsPrecommitsChanged{is_precommits_changed});

      reputation_repository_->change(
          peer_id, network::reputation::benefit::ROUND_MESSAGE);
    }

    if (not target_round->finalizedBlock().has_value()) {
      // Check if missed block are detected and if this is first attempt
      // (considering by definition peer id in context)
      if (not ctx->missing_blocks.empty()) {
        if (not ctx->peer_id.has_value()) {
          ctx->peer_id.emplace(peer_id);
          ctx->vote.emplace(msg);
          loadMissingBlocks();
        }
      }
      return;
    }
  }

  void GrandpaImpl::onCommitMessage(const libp2p::peer::PeerId &peer_id,
                                    const network::FullCommitMessage &msg) {
    // TODO check if height of commit less then previous one
    // if (new_commit_height < last_commit_height) {
    //   reputation_repository_->change(
    //       peer_id, network::reputation::cost::INVALID_VIEW_CHANGE);
    // }

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

      reputation_repository_->change(
          peer_id,
          msg.set_id < current_round_->voterSetId()
              ? network::reputation::cost::PAST_REJECTION
              : network::reputation::cost::FUTURE_MESSAGE);
      return;
    }

    // It is impolite to send commits which are earlier than the last commit
    // sent
    if (msg.round + kKeepRecentRounds < current_round_->roundNumber()) {
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

    if (msg.message.precommits.empty()
        or msg.message.auth_data.size() != msg.message.precommits.size()) {
      reputation_repository_->change(
          peer_id, network::reputation::cost::MALFORMED_COMMIT);
    }

    if (auto prev_round = current_round_->getPreviousRound()) {
      if (auto finalized_opt = prev_round->finalizedBlock()) {
        if (msg.message.target_number < finalized_opt->number) {
          reputation_repository_->change(
              peer_id, network::reputation::cost::PAST_REJECTION);
        }
      }
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

    // Check if commit of already finalized block
    if (block_tree_->getLastFinalized().number
        >= justification.block_info.number) {
      SL_DEBUG(
          logger_,
          "Commit with set_id={} in round={} for block {} has received from {} "
          "and ignored: justified block less then our last finalized ({})",
          msg.set_id,
          msg.round,
          BlockInfo(msg.message.target_number, msg.message.target_hash),
          peer_id,
          block_tree_->getLastFinalized().number);
      return;
    }

    auto has_direct_chain = block_tree_->hasDirectChain(
        block_tree_->getLastFinalized().hash, justification.block_info.hash);
    if (has_direct_chain) {
      auto res = applyJustification(justification.block_info, justification);
      if (res.has_value()) {
        reputation_repository_->change(
            peer_id, network::reputation::benefit::BASIC_VALIDATED_COMMIT);
        return;
      }

      if (ctx->missing_blocks.empty()) {
        SL_WARN(logger_,
                "Commit with set_id={} in round={} for block {} "
                "has received from {} and has not applied: {}",
                msg.set_id,
                msg.round,
                BlockInfo(msg.message.target_number, msg.message.target_hash),
                peer_id,
                res.error());
        return;
      }
    } else {
      ctx->missing_blocks.emplace(justification.block_info);
    }

    // Check if missed block are detected and if this is first attempt
    // (considering by definition peer id in context)
    if (not ctx->missing_blocks.empty()) {
      if (not ctx->peer_id.has_value()) {
        ctx->peer_id.emplace(peer_id);
        ctx->commit.emplace(msg);
        loadMissingBlocks();
      }
    }
  }

  outcome::result<void> GrandpaImpl::applyJustification(
      const BlockInfo &block_info, const GrandpaJustification &justification) {
    auto round_opt = selectRound(justification.round_number, std::nullopt);
    std::shared_ptr<VotingRound> round;
    bool need_to_make_round_current = false;
    if (round_opt.has_value()) {
      round = std::move(round_opt.value());
    } else {
      // This is justification for already finalized block
      if (current_round_->lastFinalizedBlock().number > block_info.number) {
        return VotingRoundError::JUSTIFICATION_FOR_BLOCK_IN_PAST;
      }

      auto authorities_opt =
          authority_manager_->authorities(block_info, IsBlockFinalized{false});
      if (!authorities_opt) {
        return VotingRoundError::NO_KNOWN_AUTHORITIES_FOR_BLOCK;
      }
      auto &authority_set = authorities_opt.value();

      auto prev_round_opt =
          selectRound(justification.round_number - 1, authority_set->id);

      if (prev_round_opt.has_value()) {
        const auto &prev_round = prev_round_opt.value();
        auto res = makeNextRound(prev_round);
        if (res.has_error()) {
          SL_DEBUG(logger_,
                   "Can't create next round to apply justification: {}",
                   res.error());
          return res.as_failure();
        }

        round = res.value();
        need_to_make_round_current = true;

        SL_DEBUG(logger_,
                 "Hop grandpa to round #{} by received justification",
                 justification.round_number);
      } else {
        MovableRoundState round_state{
            .round_number = justification.round_number,
            .last_finalized_block = current_round_->lastFinalizedBlock(),
            .votes = {},
            .finalized = block_info};

        // This is justification for non-actual round
        if (authority_set->id < current_round_->voterSetId()) {
          return VotingRoundError::JUSTIFICATION_FOR_AUTHORITY_SET_IN_PAST;
        }
        if (authority_set->id == current_round_->voterSetId()
            && justification.round_number < current_round_->roundNumber()) {
          if (not isWestendPastRound(block_tree_->getGenesisBlockHash(),
                                     justification.block_info)) {
            return VotingRoundError::JUSTIFICATION_FOR_ROUND_IN_PAST;
          }
        }

        if (authority_set->id > current_round_->voterSetId() + 1) {
          SL_WARN(logger_,
                  "Authority set on block {} with justification has id {}, "
                  "while the current round set id is {} (difference must be 1)",
                  block_info,
                  authority_set->id,
                  current_round_->voterSetId());
        }

        auto voters = std::make_shared<VoterSet>(authority_set->id);
        for (const auto &authority : authority_set->authorities) {
          auto res = voters->insert(
              primitives::GrandpaSessionKey(authority.id.id), authority.weight);
          if (res.has_error()) {
            SL_CRITICAL(logger_, "Can't make voter set: {}", res.error());
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
    }

    OUTCOME_TRY(round->applyJustification(block_info, justification));

    if (need_to_make_round_current) {
      current_round_->end();
      current_round_ = round;
    }

    tryExecuteNextRound(round);

    // if round == current round, then execution of the next round will be
    // elsewhere
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
