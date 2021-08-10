/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/grandpa_impl.hpp"

#include "clock/impl/ticker_impl.hpp"
#include "consensus/grandpa/impl/vote_crypto_provider_impl.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/impl/voting_round_error.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "scale/scale.hpp"
#include "storage/database_error.hpp"
#include "storage/predefined_keys.hpp"

using namespace std::literals::chrono_literals;

// a period of time to check key appearance when absent
constexpr auto kKeyWaitTimerDuration = 60s;

namespace kagome::consensus::grandpa {

  GrandpaImpl::GrandpaImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<Environment> environment,
      std::shared_ptr<storage::BufferStorage> storage,
      std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      const std::shared_ptr<crypto::Ed25519Keypair> &keypair,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<authority::AuthorityManager> authority_manager,
      std::shared_ptr<consensus::babe::Babe> babe)
      : app_state_manager_(std::move(app_state_manager)),
        environment_{std::move(environment)},
        storage_{std::move(storage)},
        crypto_provider_{std::move(crypto_provider)},
        grandpa_api_{std::move(grandpa_api)},
        keypair_{keypair},
        clock_{std::move(clock)},
        io_context_{std::move(io_context)},
        authority_manager_(std::move(authority_manager)),
        babe_(babe) {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(environment_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(crypto_provider_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(clock_ != nullptr);
    BOOST_ASSERT(io_context_ != nullptr);
    BOOST_ASSERT(authority_manager_ != nullptr);
    BOOST_ASSERT(babe_ != nullptr);

    app_state_manager_->takeControl(*this);
    catch_up_request_suppressed_until_ = clock_->now();

    key_wait_ticker_ =
        std::make_unique<clock::TickerImpl>(io_context_, kKeyWaitTimerDuration);
  }

  bool GrandpaImpl::prepare() {
    // Set themselves in environment
    environment_->setJustificationObserver(shared_from_this());

    // Lambda which is executed when voting round is completed.
    environment_->doOnCompleted(
        [wp = weak_from_this()](
            outcome::result<MovableRoundState> completed_round_res) {
          if (auto self = wp.lock()) {
            self->onCompletedRound(std::move(completed_round_res));
          }
        });
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
      logger_->critical("Can't get authorities: {}. Stopping grandpa execution",
                        authorities_res.error().message());
      return false;
    }
    auto &authorities = authorities_res.value();

    auto voters = std::make_shared<VoterSet>(authorities->id);
    for (const auto &authority : *authorities) {
      voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                     authority.weight);
    }

    current_round_ = makeInitialRound(round_state, std::move(voters));
    if (current_round_ == nullptr) {
      logger_->critical(
          "Next round hasn't been made. Stopping grandpa execution");
      return false;
    }

    BOOST_ASSERT(current_round_->finalizable());
    BOOST_ASSERT(current_round_->finalizedBlock() == round_state.finalized);

    // Lambda which is executed when voting round is completed.
    babe_->doOnSynchronized([wp = weak_from_this()] {
      if (auto self = wp.lock()) {
        // Planning play next round
        if (self->keypair_) {
          self->is_ready_ = true;
          self->current_round_->play();
        } else {
          self->key_wait_ticker_->asyncCallRepeatedly(
              [wp = self->weak_from_this()](auto &&ec) {
                if (auto self = wp.lock()) {
                  if (ec) {
                    if (ec.value() == boost::system::errc::operation_canceled) {
                      SL_INFO(self->logger_,
                              "key_wait_ticker {}", ec.message());
                      return;
                    }
                    SL_ERROR(self->logger_,
                             "error happened while waiting on the "
                             "key_wait_ticker: {}",
                             ec.message());
                    return;
                  }
                  SL_INFO(self->logger_, "Check if grandpa key appeared...");
                  if (self->keypair_) {
                    self->is_ready_ = true;
                    self->current_round_->play();
                    self->key_wait_ticker_->stop();
                  }
                }
              });
          self->key_wait_ticker_->start(self->key_wait_ticker_->interval());
        }
      }
    });

    executeNextRound();

    if (not current_round_) {
      return false;
    }

    return true;
  }

  void GrandpaImpl::stop() {}

  std::shared_ptr<VotingRound> GrandpaImpl::makeInitialRound(
      const MovableRoundState &round_state, std::shared_ptr<VoterSet> voters) {
    auto vote_graph = std::make_shared<VoteGraphImpl>(
        round_state.last_finalized_block, environment_);

    GrandpaConfig config{.voters = std::move(voters),
                         .round_number = round_state.round_number,
                         .duration = round_time_factor_,
                         .id = keypair_
                                   ? boost::make_optional(keypair_->public_key)
                                   : boost::none};

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
        io_context_,
        round_state);

    new_round->end();
    return new_round;
  }

  std::shared_ptr<VotingRound> GrandpaImpl::makeNextRound(
      const std::shared_ptr<VotingRound> &round) {
    BOOST_ASSERT(round->finalizedBlock().has_value());

    BlockInfo best_block = round->finalizedBlock().value();

    auto authorities_res = authority_manager_->authorities(best_block, true);
    if (not authorities_res.has_value()) {
      BOOST_ASSERT(authorities_res.error().message().empty());
    }
    auto &authorities = authorities_res.value();

    auto voters = std::make_shared<VoterSet>(authorities->id);
    for (const auto &authority : *authorities) {
      voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                     authority.weight);
    }

    const auto new_round_number =
        round->voterSetId() == voters->id() ? (round->roundNumber() + 1) : 1;

    auto vote_graph = std::make_shared<VoteGraphImpl>(best_block, environment_);

    GrandpaConfig config{.voters = std::move(voters),
                         .round_number = new_round_number,
                         .duration = round_time_factor_,
                         .id = keypair_
                                   ? boost::make_optional(keypair_->public_key)
                                   : boost::none};

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
        io_context_,
        round);
    return new_round;
  }

  std::shared_ptr<VotingRound> GrandpaImpl::selectRound(
      RoundNumber round_number) {
    std::shared_ptr<VotingRound> target_round;
    if (current_round_ && current_round_->roundNumber() == round_number) {
      target_round = current_round_;
    } else if (previous_round_
               && previous_round_->roundNumber() == round_number) {
      target_round = previous_round_;
    }
    return target_round;
  }

  outcome::result<MovableRoundState> GrandpaImpl::getLastCompletedRound()
      const {
    auto last_round_encoded_res = storage_->get(storage::kSetStateKey);

    // Saved data exists
    if (last_round_encoded_res.has_value()) {
      return scale::decode<MovableRoundState>(last_round_encoded_res.value());
    }

    // Fail at retrieve data
    if (last_round_encoded_res
        != outcome::failure(storage::DatabaseError::NOT_FOUND)) {
      return last_round_encoded_res.as_failure();
    }

    // No saved data - make from genesis
    auto genesis_hash_res = storage_->get(storage::kGenesisBlockHashLookupKey);
    if (not genesis_hash_res.has_value()) {
      logger_->critical("Can't retrieve genesis block hash: {}",
                        genesis_hash_res.error().message());
      return genesis_hash_res.as_failure();
    }

    primitives::BlockHash genesis_hash;
    std::copy(genesis_hash_res.value().begin(),
              genesis_hash_res.value().end(),
              genesis_hash.begin());

    return MovableRoundState{.round_number = 0,
                             .last_finalized_block = {0, genesis_hash},
                             .votes = {},
                             .finalized = {{0, genesis_hash}}};
  }

  void GrandpaImpl::executeNextRound() {
    previous_round_.swap(current_round_);
    previous_round_->end();
    current_round_ = makeNextRound(previous_round_);
    if (is_ready_) {
      current_round_->play();
    }
  }

  void GrandpaImpl::onNeighborMessage(
      const libp2p::peer::PeerId &peer_id,
      const network::GrandpaNeighborMessage &msg) {
    SL_DEBUG(logger_,
             "NeighborMessage has received from {}: "
             "voter_set_id={} round={} last_finalized={}",
             peer_id.toBase58(),
             msg.voter_set_id,
             msg.round_number,
             msg.last_finalized);
  }

  void GrandpaImpl::onCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                                     const network::CatchUpRequest &msg) {
    if (not is_ready_) {
      return;
    }
    if (previous_round_ == nullptr) {
      SL_DEBUG(
          logger_,
          "Catch-up request (since round #{}) received from {} was rejected: "
          "previous round is dummy yet",
          msg.round_number,
          peer_id.toBase58());
      return;
    }
    if (previous_round_->voterSetId() != msg.voter_set_id) {
      // Catching up of different set
      SL_DEBUG(
          logger_,
          "Catch-up request (since round #{}) received from {} was rejected: "
          "voter set is different",
          msg.round_number,
          peer_id.toBase58());
      return;
    }
    if (previous_round_->roundNumber() < msg.round_number) {
      // Catching up in to the past
      SL_DEBUG(
          logger_,
          "Catch-up request (since round #{}) received from {} was rejected: "
          "catching up in to the past",
          msg.round_number,
          peer_id.toBase58());
      return;
    }
    if (current_round_->roundNumber() + 2 <= msg.round_number) {
      if (catch_up_request_suppressed_until_ > clock_->now()) {
        catch_up_request_suppressed_until_ =
            clock_->now() + catch_up_request_suppression_duration_;
        current_round_->doCatchUpRequest(peer_id);
      }
      return;
    }
    if (not previous_round_->completable()) {
      SL_DEBUG(
          logger_,
          "Catch-up request (since round #{}) received from {} was rejected: "
          "round is not completable",
          msg.round_number,
          peer_id.toBase58());
      return;
    }
    if (not previous_round_->finalizable()) {
      SL_DEBUG(
          logger_,
          "Catch-up request (since round #{}) received from {} was rejected: "
          "round is not finalizable",
          msg.round_number,
          peer_id.toBase58());
      return;
    }

    SL_DEBUG(logger_,
             "Catch-up request (since round #{}) received from {}",
             msg.round_number,
             peer_id.toBase58());
    previous_round_->doCatchUpResponse(peer_id);
  }

  void GrandpaImpl::onCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                                      const network::CatchUpResponse &msg) {
    if (not is_ready_) {
      return;
    }
    BOOST_ASSERT(current_round_ != nullptr);
    if (current_round_->roundNumber() >= msg.round_number) {
      // Catching up in to the past
      SL_DEBUG(
          logger_,
          "Catch-up response (till round #{}) received from {} was rejected: "
          "catching up in to the past",
          msg.round_number,
          peer_id.toBase58());
      return;
    }
    if (current_round_->voterSetId() != msg.voter_set_id) {
      // Catching up of different set
      SL_DEBUG(
          logger_,
          "Catch-up response (till round #{}) received from {} was rejected: "
          "voter set is different",
          msg.round_number,
          peer_id.toBase58());
      return;
    }

    SL_DEBUG(logger_,
             "Catch-up response (till round #{}) received from {}",
             msg.round_number,
             peer_id.toBase58());

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

    auto authorities_res =
        authority_manager_->authorities(round_state.finalized.value(), false);
    if (not authorities_res.has_value()) {
      BOOST_ASSERT(authorities_res.error().message().empty());
    }
    auto &authorities = authorities_res.value();

    auto voters = std::make_shared<VoterSet>(authorities->id);
    for (const auto &authority : *authorities) {
      voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                     authority.weight);
    }

    auto round = makeInitialRound(round_state, std::move(voters));
    if (round == nullptr) {
      // Can't make round
      return;
    }

    if (current_round_->bestPrevoteCandidate().number
        > round->bestFinalCandidate().number) {
      // GHOST-less Catch-up
      return;
    }

    if (not round->completable()) {
      // Catch-up round is not completable
      return;
    }

    previous_round_.swap(current_round_);
    previous_round_->end();
    current_round_ = std::move(round);

    executeNextRound();

    catch_up_request_suppressed_until_ = clock_->now();
  }

  void GrandpaImpl::onVoteMessage(const libp2p::peer::PeerId &peer_id,
                                  const VoteMessage &msg) {
    if (not is_ready_) {
      return;
    }

    std::shared_ptr<VotingRound> target_round = selectRound(msg.round_number);
    if (not target_round) {
      if (current_round_->roundNumber() + 2 <= msg.round_number) {
        if (catch_up_request_suppressed_until_ > clock_->now()) {
          catch_up_request_suppressed_until_ =
              clock_->now() + catch_up_request_suppression_duration_;
          current_round_->doCatchUpRequest(peer_id);
        }
      }
      return;
    }

    // get block info
    auto blockInfo = visit_in_place(msg.vote.message, [](const auto &vote) {
      return BlockInfo(vote.number, vote.hash);
    });

    // get authorities
    const auto &weighted_authorities_res =
        grandpa_api_->authorities(primitives::BlockId(blockInfo.hash));
    if (!weighted_authorities_res.has_value()) {
      logger_->error("Can't get authorities");
      return;
    };
    auto &weighted_authorities = weighted_authorities_res.value();

    // find signer in authorities
    auto weighted_authority_it =
        std::find_if(weighted_authorities.begin(),
                     weighted_authorities.end(),
                     [&id = msg.vote.id](const auto &weighted_authority) {
                       return weighted_authority.id.id == id;
                     });

    if (weighted_authority_it == weighted_authorities.end()) {
      logger_->warn("Vote signed by unknown validator");
      return;
    };
    visit_in_place(
        msg.vote.message,
        [&target_round, &msg](const PrimaryPropose &) {
          target_round->onProposal(msg.vote);
        },
        [&target_round, &msg](const Prevote &) {
          target_round->onPrevote(msg.vote);
        },
        [&target_round, &msg](const Precommit &) {
          target_round->onPrecommit(msg.vote);
        });
  }

  void GrandpaImpl::onFinalize(const libp2p::peer::PeerId &peer_id,
                               const network::FullCommitMessage &fin) {
    SL_DEBUG(logger_,
             "Finalization has received from peer #{} with identity {} for "
             "block #{} with hash {}",
             fin.set_id,
             peer_id.toBase58(),
             fin.message.target_number,
             fin.message.target_hash.toHex());

    GrandpaJustification justification{
        .round_number = fin.round,
        .block_info =
            BlockInfo(fin.message.target_number, fin.message.target_hash)};
    for (size_t i = 0; i < fin.message.precommits.size(); ++i) {
      SignedPrecommit commit;
      commit.message = fin.message.precommits[i];
      commit.signature = fin.message.auth_data[i].first;
      commit.id = fin.message.auth_data[i].second;
      justification.items.push_back(commit);
    }

    if (not std::all_of(fin.message.precommits.begin(),
                        fin.message.precommits.end(),
                        [&fin](const auto &el) {
                          return el.hash == fin.message.target_hash
                                 && el.number == fin.message.target_number;
                        })) {
      logger_->warn("Block does not correspond to the votes");
      return;
    }

    if (not is_ready_) {
      // grandpa not initialized, we just finalize block then
      auto res =
          environment_->finalize(justification.block_info.hash, justification);
      if (not res.has_value()) {
        logger_->warn("Can't make simple block finalization: {}",
                      res.error().message());
      }
      return;
    }

    auto res = applyJustification(justification.block_info, justification);
    if (not res.has_value()) {
      logger_->warn("Fin message is not applied: {}", res.error().message());
      return;
    }
  }

  outcome::result<void> GrandpaImpl::applyJustification(
      const BlockInfo &block_info, const GrandpaJustification &justification) {
    if (auto target_round = selectRound(justification.round_number)) {
      return target_round->applyJustification(block_info, justification);
    }

    if (current_round_->bestPrevoteCandidate().number > block_info.number) {
      return VotingRoundError::JUSTIFICATION_FOR_BLOCK_IN_PAST;
    }

    MovableRoundState round_state{
        .round_number = justification.round_number,
        .last_finalized_block = current_round_->lastFinalizedBlock(),
        .votes = {},
        .finalized = block_info};

    auto authorities_res =
        authority_manager_->authorities(round_state.last_finalized_block, true);
    if (not authorities_res.has_value()) {
      BOOST_ASSERT(authorities_res.error().message().empty());
    }
    auto &authorities = authorities_res.value();

    auto voters = std::make_shared<VoterSet>(authorities->id);
    for (const auto &authority : *authorities) {
      voters->insert(primitives::GrandpaSessionKey(authority.id.id),
                     authority.weight);
    }

    auto round = makeInitialRound(round_state, std::move(voters));
    assert(round);

    SL_DEBUG(logger_,
             "Rewind grandpa till round #{} by received justification",
             justification.round_number);

    OUTCOME_TRY(round->applyJustification(block_info, justification));

    previous_round_.swap(current_round_);
    previous_round_->end();
    current_round_ = std::move(round);

    executeNextRound();

    return outcome::success();
  }

  void GrandpaImpl::onCompletedRound(
      outcome::result<MovableRoundState> round_state_res) {
    if (not round_state_res) {
      SL_DEBUG(logger_,
               "Grandpa round was not finalized: {}",
               round_state_res.error().message());
      return;
    }

    const auto &round_state = round_state_res.value();

    logger_->debug(
        "Save state of finalized round #{}: finalized={}, finalizing={}",
        round_state.round_number,
        round_state.last_finalized_block.number,
        round_state.finalized.value().number);

    SL_DEBUG(
        logger_, "Event OnCompleted for round #{}", round_state.round_number);

    if (auto put_res =
            storage_->put(storage::kSetStateKey,
                          common::Buffer(scale::encode(round_state).value()));
        not put_res) {
      logger_->error("New round state was not added to the storage");
      return;
    }

    BOOST_ASSERT(storage_->get(storage::kSetStateKey));
  }
}  // namespace kagome::consensus::grandpa
