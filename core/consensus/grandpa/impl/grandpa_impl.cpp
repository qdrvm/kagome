/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/grandpa_impl.hpp"

#include <utility>

#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/common/final_action.hpp>

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/main_thread_pool.hpp"
#include "common/tagged.hpp"
#include "consensus/grandpa/authority_manager.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/grandpa_config.hpp"
#include "consensus/grandpa/grandpa_context.hpp"
#include "consensus/grandpa/impl/grandpa_thread_pool.hpp"
#include "consensus/grandpa/impl/vote_crypto_provider_impl.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "consensus/grandpa/voting_round_error.hpp"
#include "consensus/grandpa/voting_round_update.hpp"
#include "consensus/timeline/timeline.hpp"
#include "crypto/key_store/session_keys.hpp"
#include "network/peer_manager.hpp"
#include "network/reputation_repository.hpp"
#include "network/synchronizer.hpp"
#include "storage/predefined_keys.hpp"
#include "utils/pool_handler.hpp"
#include "utils/pool_handler_ready_make.hpp"
#include "utils/retain_if.hpp"

namespace {
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

  // https://github.com/paritytech/polkadot/pull/6217
  constexpr std::chrono::milliseconds kGossipDuration{1000};

  inline auto historicalVotesKey(AuthoritySetId set, RoundNumber round) {
    auto key = storage::kGrandpaHistoricalVotesPrefix;
    key.putUint64(set);
    key.putUint64(round);
    return key;
  }

  GrandpaImpl::GrandpaImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<Environment> environment,
      std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
      std::shared_ptr<crypto::SessionKeys> session_keys,
      std::shared_ptr<AuthorityManager> authority_manager,
      std::shared_ptr<network::Synchronizer> synchronizer,
      std::shared_ptr<network::PeerManager> peer_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<network::ReputationRepository> reputation_repository,
      LazySPtr<Timeline> timeline,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
      storage::SpacedStorage &db,
      common::MainThreadPool &main_thread_pool,
      GrandpaThreadPool &grandpa_thread_pool)
      : round_time_factor_{kGossipDuration},
        hasher_{std::move(hasher)},
        environment_{std::move(environment)},
        crypto_provider_{std::move(crypto_provider)},
        session_keys_{std::move(session_keys)},
        authority_manager_(std::move(authority_manager)),
        synchronizer_(std::move(synchronizer)),
        peer_manager_(std::move(peer_manager)),
        block_tree_(std::move(block_tree)),
        reputation_repository_(std::move(reputation_repository)),
        timeline_{std::move(timeline)},
        chain_sub_{chain_sub_engine},
        db_{db.getSpace(storage::Space::kDefault)},
        main_pool_handler_{main_thread_pool.handler(*app_state_manager)},
        grandpa_pool_handler_{poolHandlerReadyMake(
            this, app_state_manager, grandpa_thread_pool, logger_)},
        scheduler_{std::make_shared<libp2p::basic::SchedulerImpl>(
            std::make_shared<libp2p::basic::AsioSchedulerBackend>(
                grandpa_thread_pool.io_context()),
            libp2p::basic::Scheduler::Config{})} {
    BOOST_ASSERT(environment_ != nullptr);
    BOOST_ASSERT(crypto_provider_ != nullptr);
    BOOST_ASSERT(authority_manager_ != nullptr);
    BOOST_ASSERT(synchronizer_ != nullptr);
    BOOST_ASSERT(peer_manager_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(reputation_repository_ != nullptr);
    BOOST_ASSERT(main_pool_handler_ != nullptr);
    BOOST_ASSERT(grandpa_pool_handler_ != nullptr);

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

  bool GrandpaImpl::tryStart() {
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
        round_state.last_finalized_block, false);
    if (not authorities_res.has_value()) {
      logger_->critical(
          "Can't retrieve authorities for block {}. Stopping grandpa execution",
          round_state.last_finalized_block);
      return false;
    }
    auto &authority_set = authorities_res.value();

    auto voters_res = VoterSet::make(*authority_set);
    if (not voters_res) {
      logger_->critical("Can't make voter set: {}. Stopping grandpa execution",
                        voters_res.error());
      return false;
    }
    auto &voters = voters_res.value();

    current_round_ =
        makeInitialRound(round_state, std::move(voters), *authority_set);
    BOOST_ASSERT(current_round_ != nullptr);

    if (not current_round_->finalizedBlock().has_value()) {
      logger_->critical(
          "Initial round must be finalized, but it is not. "
          "Stopping grandpa execution");
      return false;
    }

    // Timer to send neighbor message if round does not change long time (1 min)
    setTimerFallback();

    tryExecuteNextRound(current_round_);

    chain_sub_.onHead(
        [weak{weak_from_this()}](const primitives::BlockHeader &block) {
          if (auto self = weak.lock()) {
            self->onHead(block.blockInfo());
          }
        });
    return true;
  }

  void GrandpaImpl::stop() {
    fallback_timer_handle_.reset();
  }

  std::shared_ptr<VotingRound> GrandpaImpl::makeInitialRound(
      const MovableRoundState &round_state,
      std::shared_ptr<VoterSet> voters,
      const AuthoritySet &authority_set) {
    auto vote_graph = std::make_shared<VoteGraphImpl>(
        round_state.last_finalized_block, voters, environment_);

    auto keypair = session_keys_->getGranKeyPair(authority_set);

    GrandpaConfig config{
        .voters = std::move(voters),
        .round_number = round_state.round_number,
        .duration = round_time_factor_,
        .id = keypair ? std::make_optional(keypair->public_key) : std::nullopt,
    };

    auto vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair, crypto_provider_, round_state.round_number, config.voters);

    auto new_round = std::make_shared<VotingRoundImpl>(
        shared_from_this(),
        std::move(config),
        hasher_,
        environment_,
        std::move(vote_crypto_provider),
        std::make_shared<VoteTrackerImpl>(),  // Prevote tracker
        std::make_shared<VoteTrackerImpl>(),  // Precommit tracker
        std::move(vote_graph),
        scheduler_,
        round_state);
    applyHistoricalVotes(*new_round);

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

    auto voters_res = VoterSet::make(*authority_set);
    if (not voters_res) {
      SL_WARN(logger_, "Can't make voter set: {}", voters_res.error());
      return voters_res.error();
    }
    auto &voters = voters_res.value();

    const auto new_round_number =
        round->voterSetId() == voters->id() ? (round->roundNumber() + 1) : 1;

    auto vote_graph =
        std::make_shared<VoteGraphImpl>(best_block, voters, environment_);

    auto keypair = session_keys_->getGranKeyPair(*authority_set);

    GrandpaConfig config{
        .voters = std::move(voters),
        .round_number = new_round_number,
        .duration = round_time_factor_,
        .id = keypair ? std::make_optional(keypair->public_key) : std::nullopt,
    };

    auto vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair, crypto_provider_, new_round_number, config.voters);

    auto new_round = std::make_shared<VotingRoundImpl>(
        shared_from_this(),
        std::move(config),
        hasher_,
        environment_,
        std::move(vote_crypto_provider),
        std::make_shared<VoteTrackerImpl>(),  // Prevote tracker
        std::make_shared<VoteTrackerImpl>(),  // Precommit tracker
        std::move(vote_graph),
        scheduler_,
        round);
    applyHistoricalVotes(*new_round);
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
    REINVOKE(*grandpa_pool_handler_, tryExecuteNextRound, prev_round);
    if (current_round_ != prev_round) {
      return;
    }

    auto res = makeNextRound(current_round_);
    if (res.has_error()) {
      SL_WARN(logger_, "Next round was not created: {}", res.error());
      return;
    }
    BOOST_ASSERT(res.value() != nullptr);
    current_round_ = std::move(res.value());

    setTimerFallback();

    // Truncate chain of rounds
    size_t i = 0;
    for (auto round = current_round_; round != nullptr;
         round = round->getPreviousRound()) {
      if (++i >= kKeepRecentRounds) {
        round->forgetPreviousRound();
      }
    }

    metric_highest_round_->set(current_round_->roundNumber());
    if (current_round_->hasKeypair()) {
      current_round_->play();
    } else {
      auto round = std::dynamic_pointer_cast<VotingRoundImpl>(current_round_);
      if (round) {
        round->sendNeighborMessage();
      }
    }
  }

  void GrandpaImpl::updateNextRound(RoundNumber round_number) {
    REINVOKE(*grandpa_pool_handler_, updateNextRound, round_number);
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
      std::optional<network::PeerStateCompact> &&info,
      network::GrandpaNeighborMessage &&msg) {
    REINVOKE(*grandpa_pool_handler_,
             onNeighborMessage,
             peer_id,
             std::move(info),
             std::move(msg));

    BOOST_ASSERT(grandpa_pool_handler_->isInCurrentThread());
    SL_DEBUG(logger_,
             "NeighborMessage set_id={} round={} last_finalized={} "
             "has received from {}",
             msg.voter_set_id,
             msg.round_number,
             msg.last_finalized,
             peer_id);

    std::optional<VoterSetId> info_set;
    std::optional<RoundNumber> info_round;
    // copy values before `updatePeerState`

    if (info) {
      info_set = info->set_id;
      info_round = info->round_number;
    }

    bool reputation_changed = false;
    if (info_set and info_round) {
      const auto prev_set_id = *info_set;
      const auto prev_round_number = *info_round;

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

    // If peer just reached one of recent round, then share known votes
    if (msg.voter_set_id != info_set or not info_round
        or msg.round_number > *info_round) {
      if (auto opt_round = selectRound(msg.round_number, msg.voter_set_id);
          opt_round.has_value()) {
        auto &round = opt_round.value();
        environment_->sendState(peer_id, round->state(), msg.voter_set_id);
      }
    }

    if (not timeline_.get()->wasSynchronized()) {
      return;
    }

    // If peer has the same voter set id
    if (msg.voter_set_id == current_round_->voterSetId()) {
      // Check if needed to catch-up peer, then do that
      if (msg.round_number
          >= current_round_->roundNumber() + kCatchUpThreshold) {
        // Do catch-up only when another one is not in progress
        if (not pending_catchup_request_.has_value()) {
          environment_->onCatchUpRequested(
              peer_id, msg.voter_set_id, msg.round_number - 1);
          if (pending_catchup_request_.has_value()) {
            SL_WARN(logger_,
                    "Catch up request pending, but another one has done");
          }
          pending_catchup_request_.emplace(
              peer_id,
              network::CatchUpRequest{msg.round_number - 1, msg.voter_set_id});
          catchup_request_timer_handle_ = scheduler_->scheduleWithHandle(
              [wp{weak_from_this()}] {
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
      return;
    }

    // Ignore peer whose voter set id lower than our current
    if (msg.voter_set_id < current_round_->voterSetId()) {
      return;
    }
  }

  void GrandpaImpl::onCatchUpRequest(
      const libp2p::peer::PeerId &peer_id,
      std::optional<network::PeerStateCompact> &&info_opt,
      network::CatchUpRequest &&msg) {
    REINVOKE(*grandpa_pool_handler_,
             onCatchUpRequest,
             peer_id,
             std::move(info_opt),
             std::move(msg));

    if (not info_opt.has_value() or not info_opt->set_id.has_value()
        or not info_opt->round_number.has_value()) {
      SL_DEBUG(logger_,
               "Catch-up request to round #{} received from {} was rejected: "
               "we are not have our view about remote peer",
               msg.round_number,
               peer_id);
      reputation_repository_->change(
          peer_id, network::reputation::cost::OUT_OF_SCOPE_MESSAGE);
      return;
    }
    const auto &info = *info_opt;

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
                                      network::CatchUpResponse &&msg) {
    onCatchUpResponse(peer_id, std::move(msg), true);
  }

  void GrandpaImpl::onCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                                      network::CatchUpResponse &&msg,
                                      bool allow_missing_blocks) {
    REINVOKE(*grandpa_pool_handler_,
             onCatchUpResponse,
             peer_id,
             msg,
             allow_missing_blocks);

    bool need_cleanup_when_exiting_scope = false;

    if (allow_missing_blocks) {
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

    ::libp2p::common::FinalAction cleanup([&] {
      if (need_cleanup_when_exiting_scope) {
        catchup_request_timer_handle_.reset();
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

    auto round = current_round_;
    auto new_round = msg.round_number > current_round_->roundNumber();
    if (new_round) {
      MovableRoundState round_state{
          .round_number = msg.round_number,
          .last_finalized_block = current_round_->lastFinalizedBlock(),
          .votes = {},
          .finalized = msg.best_final_candidate};

      auto authorities_opt = authority_manager_->authorities(
          round_state.finalized.value(), IsBlockFinalized{false});
      if (!authorities_opt) {
        SL_WARN(logger_,
                "Can't retrieve authorities for finalized block {}",
                round_state.finalized.value());
        return;
      }
      auto &authority_set = authorities_opt.value();

      auto voters_res = VoterSet::make(*authority_set);
      if (not voters_res) {
        SL_WARN(logger_, "Can't make voter set: {}", voters_res.error());
        return;
      }
      auto &voters = voters_res.value();

      round = makeInitialRound(round_state, std::move(voters), *authority_set);
    }

    GrandpaContext grandpa_context;
    VotingRoundUpdate update{*round, grandpa_context};
    for (auto &vote : msg.prevote_justification) {
      update.vote(vote);
    }
    for (auto &vote : msg.precommit_justification) {
      update.vote(vote);
    }
    update.update();

    SL_DEBUG(logger_, "Catch-up response applied");

    if (not round->completable()
        and (not new_round or not round->finalizedBlock())) {
      // Met unknown voter - cost reputation
      if (grandpa_context.unknown_voter_counter > 0) {
        reputation_repository_->change(
            peer_id,
            network::reputation::cost::UNKNOWN_VOTER
                * grandpa_context.unknown_voter_counter);
      }
      // Met invalid signature - cost reputation
      if (grandpa_context.invalid_signature_counter > 0) {
        reputation_repository_->change(
            peer_id,
            network::reputation::cost::BAD_CATCHUP_RESPONSE
                * grandpa_context.checked_signature_counter);
      }
      if (allow_missing_blocks) {
        loadMissingBlocks({peer_id,
                           std::move(msg),
                           std::move(grandpa_context.missing_blocks)});
      }
      return;
    }

    if (new_round) {
      current_round_->end();
      current_round_ = std::move(round);
    }

    tryExecuteNextRound(current_round_);

    reputation_repository_->change(
        peer_id, network::reputation::benefit::BASIC_VALIDATED_CATCH_UP);
  }

  void GrandpaImpl::onVoteMessage(
      const libp2p::peer::PeerId &peer_id,
      std::optional<network::PeerStateCompact> &&info,
      VoteMessage &&msg) {
    onVoteMessage(peer_id, std::move(info), std::move(msg), true);
  }

  void GrandpaImpl::onVoteMessage(
      const libp2p::peer::PeerId &peer_id,
      std::optional<network::PeerStateCompact> &&info,
      VoteMessage &&msg,
      bool allow_missing_blocks) {
    REINVOKE(*grandpa_pool_handler_,
             onVoteMessage,
             peer_id,
             std::move(info),
             msg,
             allow_missing_blocks);

    if (allow_missing_blocks) {
      // Skip message processing if same vote was already observed
      if (votes_cache_.contains(msg)) {
        return;
      }
      votes_cache_.put(msg);
    }

    if (not info.has_value() or not info->set_id.has_value()
        or not info->round_number.has_value()) {
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
    } else if (msg.round_number + 1 < current_round_->roundNumber()) {
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
    if (msg.round_number > current_round_->roundNumber() + 1) {
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

    GrandpaContext grandpa_context;
    VotingRoundUpdate update{*target_round, grandpa_context, true};
    update.vote(msg.vote);
    update.update();

    // Met invalid signature - cost reputation
    if (grandpa_context.invalid_signature_counter > 0) {
      reputation_repository_->change(
          peer_id,
          network::reputation::cost::BAD_SIGNATURE
              * grandpa_context.checked_signature_counter);
    }

    // Met unknown voter - cost reputation
    if (grandpa_context.unknown_voter_counter > 0) {
      reputation_repository_->change(
          peer_id,
          network::reputation::cost::UNKNOWN_VOTER
              * grandpa_context.unknown_voter_counter);
    }

    if (update.changed()) {
      reputation_repository_->change(
          peer_id, network::reputation::benefit::ROUND_MESSAGE);
    }

    if (not target_round->finalizedBlock().has_value()) {
      if (allow_missing_blocks) {
        loadMissingBlocks({peer_id,
                           std::move(msg),
                           std::move(grandpa_context.missing_blocks)});
      }
      return;
    }
  }

  void GrandpaImpl::onCommitMessage(const libp2p::peer::PeerId &peer_id,
                                    network::FullCommitMessage &&msg) {
    onCommitMessage(peer_id, std::move(msg), true);
  }

  void GrandpaImpl::onCommitMessage(const libp2p::peer::PeerId &peer_id,
                                    network::FullCommitMessage &&msg,
                                    bool allow_missing_blocks) {
    REINVOKE(*grandpa_pool_handler_,
             onCommitMessage,
             peer_id,
             msg,
             allow_missing_blocks);

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

    if (allow_missing_blocks) {
      MissingBlocks missing_blocks;
      for (auto &vote : justification.items) {
        if (not block_tree_->has(vote.getBlockHash())) {
          missing_blocks.emplace(vote.getBlockInfo());
        }
      }
      if (not missing_blocks.empty()) {
        loadMissingBlocks({peer_id, std::move(msg), std::move(missing_blocks)});
        return;
      }
    }

    applyJustification(
        justification, [wself{weak_from_this()}, peer_id](auto &&res) mutable {
          if (auto self = wself.lock()) {
            if (res.has_value()) {
              self->reputation_repository_->change(
                  peer_id,
                  network::reputation::benefit::BASIC_VALIDATED_COMMIT);
              return;
            }
          }
        });
  }

  void GrandpaImpl::callbackCall(ApplyJustificationCb &&callback,
                                 outcome::result<void> &&result) {
    main_pool_handler_->execute(
        [callback{std::move(callback)}, result{std::move(result)}]() mutable {
          callback(std::move(result));
        });
  }

  outcome::result<void> GrandpaImpl::verifyJustification(
      const GrandpaJustification &justification,
      const AuthoritySet &authorities) {
    auto voters = VoterSet::make(authorities).value();
    MovableRoundState state;
    state.round_number = justification.round_number;
    auto round = std::make_shared<VotingRoundImpl>(
        shared_from_this(),
        GrandpaConfig{voters, justification.round_number, {}, {}},
        hasher_,
        environment_,
        std::make_shared<VoteCryptoProviderImpl>(
            nullptr, crypto_provider_, justification.round_number, voters),
        std::make_shared<VoteTrackerImpl>(),
        std::make_shared<VoteTrackerImpl>(),
        std::make_shared<VoteGraphImpl>(
            primitives::BlockInfo{}, voters, environment_),
        scheduler_,
        state);
    return round->validatePrecommitJustification(justification);
  }

  void GrandpaImpl::applyJustification(
      const GrandpaJustification &justification,
      ApplyJustificationCb &&callback) {
    REINVOKE(*grandpa_pool_handler_,
             applyJustification,
             justification,
             std::move(callback));
    auto authorities_opt = authority_manager_->authorities(
        justification.block_info, IsBlockFinalized{false});
    if (not authorities_opt) {
      callbackCall(std::move(callback),
                   VotingRoundError::NO_KNOWN_AUTHORITIES_FOR_BLOCK);
      return;
    }
    auto &authority_set = authorities_opt.value();
    auto round_opt = selectRound(justification.round_number, authority_set->id);
    if (not round_opt
        and std::pair{authority_set->id, justification.round_number}
                < std::pair{current_round_->voterSetId(),
                            current_round_->roundNumber()}) {
      auto r = verifyJustification(justification, *authority_set);
      if (r.has_error()) {
        SL_WARN(logger_,
                "verify justification block {} set {} round {}: {}",
                justification.block_info.number,
                authority_set->id,
                justification.round_number,
                r.error());
      } else {
        r = environment_->finalize(authority_set->id, justification);
      }
      callbackCall(std::move(callback), std::move(r));
      return;
    }
    std::shared_ptr<VotingRound> round;
    bool need_to_make_round_current = false;
    if (round_opt.has_value()) {
      round = std::move(round_opt.value());
    } else {
      // This is justification for already finalized block
      if (current_round_->lastFinalizedBlock().number
          > justification.block_info.number) {
        callbackCall(std::move(callback),
                     VotingRoundError::JUSTIFICATION_FOR_BLOCK_IN_PAST);
        return;
      }

      auto prev_round_opt =
          selectRound(justification.round_number - 1, authority_set->id);

      if (prev_round_opt.has_value()) {
        const auto &prev_round = prev_round_opt.value();
        auto res = makeNextRound(prev_round);
        if (res.has_error()) {
          SL_DEBUG(logger_,
                   "Can't create next round to apply justification: {}",
                   res.error());
          callbackCall(std::move(callback), res.as_failure());
          return;
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
            .finalized = justification.block_info,
        };

        // This is justification for non-actual round
        if (authority_set->id < current_round_->voterSetId()) {
          callbackCall(
              std::move(callback),
              VotingRoundError::JUSTIFICATION_FOR_AUTHORITY_SET_IN_PAST);
          return;
        }
        if (authority_set->id == current_round_->voterSetId()
            && justification.round_number < current_round_->roundNumber()) {
          if (not isWestendPastRound(block_tree_->getGenesisBlockHash(),
                                     justification.block_info)) {
            callbackCall(std::move(callback),
                         VotingRoundError::JUSTIFICATION_FOR_ROUND_IN_PAST);
            return;
          }
        }

        if (authority_set->id > current_round_->voterSetId() + 1) {
          SL_WARN(logger_,
                  "Authority set on block {} with justification has id {}, "
                  "while the current round set id is {} (difference must be 1)",
                  justification.block_info,
                  authority_set->id,
                  current_round_->voterSetId());
        }

        auto voters_res = VoterSet::make(*authority_set);
        if (not voters_res) {
          SL_CRITICAL(logger_, "Can't make voter set: {}", voters_res.error());
          callbackCall(std::move(callback), voters_res.error());
          return;
        }
        auto &voters = voters_res.value();

        round =
            makeInitialRound(round_state, std::move(voters), *authority_set);
        need_to_make_round_current = true;
        BOOST_ASSERT(round);

        SL_DEBUG(logger_,
                 "Rewind grandpa till round #{} by received justification",
                 justification.round_number);
      }
    }

    if (auto r = round->applyJustification(justification); r.has_error()) {
      callbackCall(std::move(callback), r.as_failure());
      return;
    }

    if (need_to_make_round_current) {
      current_round_->end();
      current_round_ = round;
    }

    tryExecuteNextRound(round);

    // if round == current round, then execution of the next round will be
    // elsewhere
    callbackCall(std::move(callback), outcome::success());
  }

  void GrandpaImpl::reload() {
    REINVOKE(*grandpa_pool_handler_, reload);
    if (not tryStart()) {
      SL_CRITICAL(logger_, "reload failed");
    }
  }

  void GrandpaImpl::loadMissingBlocks(WaitingBlock &&waiting) {
    if (waiting.blocks.empty()) {
      return;
    }
    if (not timeline_.get()->wasSynchronized()) {
      return;
    }
    main_pool_handler_->execute(
        [s{synchronizer_}, peer{waiting.peer}, blocks{waiting.blocks}] {
          for (auto &block : blocks) {
            s->syncByBlockInfo(block, peer, nullptr, false);
          }
        });
    waiting_blocks_.emplace_back(std::move(waiting));
    pruneWaitingBlocks();
  }

  void GrandpaImpl::onHead(const primitives::BlockInfo &block) {
    if (not timeline_.get()->wasSynchronized()) {
      return;
    }
    REINVOKE(*grandpa_pool_handler_, onHead, block);
    auto f = [&](WaitingBlock &waiting) {
      if (waiting.blocks.erase(block) == 0) {
        return true;
      }
      if (not waiting.blocks.empty()) {
        return true;
      }
      auto f = [weak{weak_from_this()}, waiting{std::move(waiting)}]() mutable {
        auto self = weak.lock();
        if (not self) {
          return;
        }
        auto &peer = waiting.peer;
        if (auto msg = boost::get<network::VoteMessage>(&waiting.msg)) {
          auto info = self->peer_manager_->getPeerState(peer);
          self->onVoteMessage(
              peer, compactFromRefToOwn(info), std::move(*msg), false);
        } else if (auto msg =
                       boost::get<network::CatchUpResponse>(&waiting.msg)) {
          self->onCatchUpResponse(peer, std::move(*msg), false);
        } else {
          auto &commit = boost::get<network::FullCommitMessage>(waiting.msg);
          self->onCommitMessage(peer, std::move(commit), false);
        }
      };
      grandpa_pool_handler_->execute(std::move(f));
      return false;
    };
    retain_if(waiting_blocks_, f);
    pruneWaitingBlocks();
  }

  void GrandpaImpl::pruneWaitingBlocks() {
    auto round = [&](VoterSetId set, RoundNumber round) {
      for (auto p = current_round_; p; p = p->getPreviousRound()) {
        if (p->voterSetId() == set and p->roundNumber() == round) {
          return true;
        }
      }
      return false;
    };
    auto f = [&](const WaitingBlock &waiting) {
      if (auto msg = boost::get<network::CatchUpResponse>(&waiting.msg)) {
        return round(msg->voter_set_id, msg->round_number);
      }
      if (auto msg = boost::get<network::FullCommitMessage>(&waiting.msg)) {
        return round(msg->set_id, msg->round);
      }
      auto &msg = boost::get<network::VoteMessage>(waiting.msg);
      return round(msg.counter, msg.round_number);
    };
    retain_if(waiting_blocks_, f);
  }

  void GrandpaImpl::saveHistoricalVote(AuthoritySetId set,
                                       RoundNumber round,
                                       const SignedMessage &vote,
                                       bool set_index) {
    REINVOKE(*grandpa_pool_handler_,
             saveHistoricalVote,
             set,
             round,
             vote,
             set_index);
    auto &[votes, dirty] = historicalVotes(set, round);
    if (std::find(votes.seen.begin(), votes.seen.end(), vote)
        != votes.seen.end()) {
      return;
    }
    if (set_index) {
      auto *index = vote.is<Prevote>()   ? &votes.prevote_idx
                  : vote.is<Precommit>() ? &votes.precommit_idx
                                         : nullptr;
      if (index and not *index) {
        *index = votes.seen.size();
      }
    }
    votes.seen.emplace_back(vote);
    dirty = true;
    if (writing_historical_votes_) {
      return;
    }
    writing_historical_votes_ = true;
    grandpa_pool_handler_->execute([weak_self{weak_from_this()}] {
      auto self = weak_self.lock();
      if (not self) {
        return;
      }
      self->writeHistoricalVotes();
    });
  }

  void GrandpaImpl::writeHistoricalVotes() {
    writing_historical_votes_ = false;
    historical_votes_.forEach(
        [&](const HistoricalVotesKey &key, HistoricalVotesDirty &cache) {
          if (not cache.second) {
            return;
          }
          cache.second = false;
          std::ignore = db_->put(historicalVotesKey(key.first, key.second),
                                 scale::encode(cache.first).value());
        });
  }

  GrandpaImpl::HistoricalVotesDirty &GrandpaImpl::historicalVotes(
      AuthoritySetId set, RoundNumber round) {
    auto key = std::make_pair(set, round);
    auto cache = historical_votes_.get(key);
    if (not cache) {
      cache = historical_votes_.put(key, {{}, false});
      if (auto r = db_->get(historicalVotesKey(set, round))) {
        if (auto r2 = scale::decode<HistoricalVotes>(r.value())) {
          cache->get().first = std::move(r2.value());
        } else {
          SL_ERROR(logger_,
                   "historicalVotes(set={}, round={}): decode error",
                   set,
                   round);
        }
      }
    }
    return cache->get();
  }

  void GrandpaImpl::applyHistoricalVotes(VotingRound &round) {
    auto &votes =
        historicalVotes(round.voterSetId(), round.roundNumber()).first;
    VotingRoundUpdate update{round};
    for (auto &vote : votes.seen) {
      update.vote(vote);
    }
    update.update();
  }

  void GrandpaImpl::setTimerFallback() {
    fallback_timer_handle_ = scheduler_->scheduleWithHandle(
        [weak_self{weak_from_this()}] {
          auto self = weak_self.lock();
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
          self->setTimerFallback();
        },
        std::chrono::minutes(1));
  }
}  // namespace kagome::consensus::grandpa
