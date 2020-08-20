/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/grandpa_impl.hpp"

#include <boost/asio/post.hpp>
#include <storage/database_error.hpp>

#include "consensus/grandpa/impl/vote_crypto_provider_impl.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::consensus::grandpa {

  static size_t round_id = 0;

  GrandpaImpl::GrandpaImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<Environment> environment,
      std::shared_ptr<storage::BufferStorage> storage,
      std::shared_ptr<crypto::ED25519Provider> crypto_provider,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      const crypto::ED25519Keypair &keypair,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<authority::AuthorityManager> authority_manager)
      : app_state_manager_(std::move(app_state_manager)),
        environment_{std::move(environment)},
        storage_{std::move(storage)},
        crypto_provider_{std::move(crypto_provider)},
        grandpa_api_{std::move(grandpa_api)},
        keypair_{keypair},
        clock_{std::move(clock)},
        io_context_{std::move(io_context)},
        authority_manager_(std::move(authority_manager)) {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(environment_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(crypto_provider_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(clock_ != nullptr);
    BOOST_ASSERT(io_context_ != nullptr);
    BOOST_ASSERT(authority_manager_ != nullptr);

    app_state_manager_->takeControl(*this);
  }

  bool GrandpaImpl::prepare() {
    // Lambda which is executed when voting round is completed.
    environment_->doOnCompleted(
        [wp = weak_from_this()](
            outcome::result<CompletedRound> completed_round_res) {
          if (auto self = wp.lock()) {
            self->onCompletedRound(std::move(completed_round_res));
          }
        });
    return true;
  }

  bool GrandpaImpl::start() {
    // Obtain last completed round
    auto last_round_res = getLastCompletedRound();
    if (not last_round_res.has_value()) {
      // No saved data
      if (last_round_res
          != outcome::failure(storage::DatabaseError::NOT_FOUND)) {
        logger_->critical(
            "Can't retrieve last round data: {}. Stopping grandpa execution",
            last_round_res.error().message());
        return false;
      }
    }

    auto &[last_round_number, last_round_state] = last_round_res.value();

    logger_->debug("Grandpa is starting with round #{}", last_round_number + 1);

    current_round_ = makeInitialRound(last_round_number, last_round_state);

    // Planning play round
    boost::asio::post(*io_context_, [wp = current_round_->weak_from_this()] {
      if (auto round = wp.lock()) {
        round->play();
      }
    });

    return true;
  }

  void GrandpaImpl::stop() {}

  std::shared_ptr<VotingRound> GrandpaImpl::makeInitialRound(
      RoundNumber previous_round_number,
      std::shared_ptr<const RoundState> previous_round_state) {
    assert(previous_round_state != nullptr);

    // Obtain grandpa voters
    auto voters_res = getVoters();
    if (not voters_res.has_value()) {
      logger_->critical("Can't retrieve voters: {}. Stopping grandpa execution",
                        voters_res.error().message());
      return {};
    }
    const auto &voters = voters_res.value();
    if (voters->empty()) {
      logger_->critical("Voters are empty. Stopping grandpa execution");
      return {};
    }

    const auto new_round_number = previous_round_number + 1;

    auto vote_graph = std::make_shared<VoteGraphImpl>(
        previous_round_state->finalized.value_or(
            previous_round_state->last_finalized_block),
        environment_);

    GrandpaConfig config{
        .voters = voters,
        .round_number = new_round_number,
        .duration = std::chrono::milliseconds(
            333),  // Note: Duration value was gotten from substrate:
                   // https://github.com/paritytech/substrate/blob/efbac7be80c6e8988a25339061078d3e300f132d/bin/node-template/node/src/service.rs#L166
        .peer_id = keypair_.public_key};

    auto vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair_, crypto_provider_, new_round_number, voters);

    auto new_round = std::make_shared<VotingRoundImpl>(
        shared_from_this(),
        std::move(config),
        environment_,
        std::move(vote_crypto_provider),
        std::make_shared<VoteTrackerImpl>(),  // Prevote tracker
        std::make_shared<VoteTrackerImpl>(),  // Precommit tracker
        std::move(vote_graph),
        clock_,
        io_context_,
        nullptr,
        std::move(previous_round_state));

    return new_round;
  }

  std::shared_ptr<VotingRound> GrandpaImpl::makeNextRound(
      const std::shared_ptr<VotingRound> &round) {
    logger_->debug("Making next round");

    // Obtain grandpa voters
    auto voters_res = getVoters();
    if (not voters_res.has_value()) {
      logger_->critical("Can't retrieve voters: {}. Stopping grandpa execution",
                        voters_res.error().message());
      return {};
    }
    const auto &voters = voters_res.value();
    if (voters->empty()) {
      logger_->critical("Voters are empty. Stopping grandpa execution");
      return {};
    }

    const auto new_round_number = round->roundNumber() + 1;

    BlockInfo best_block = round->state()->finalized.value_or(
        round->state()->last_finalized_block);

    auto vote_graph = std::make_shared<VoteGraphImpl>(best_block, environment_);

    GrandpaConfig config{
        .voters = voters,
        .round_number = new_round_number,
        .duration = std::chrono::milliseconds(
            333),  // Note: Duration value was gotten from substrate:
                   // https://github.com/paritytech/substrate/blob/efbac7be80c6e8988a25339061078d3e300f132d/bin/node-template/node/src/service.rs#L166
        .peer_id = keypair_.public_key};

    auto vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair_, crypto_provider_, new_round_number, voters);

    auto new_round = std::make_shared<VotingRoundImpl>(
        shared_from_this(),
        std::move(config),
        environment_,
        std::move(vote_crypto_provider),
        std::make_shared<VoteTrackerImpl>(),  // Prevote tracker
        std::make_shared<VoteTrackerImpl>(),  // Precommit tracker
        std::move(vote_graph),
        clock_,
        io_context_,
        round,
        nullptr);
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

  outcome::result<std::shared_ptr<VoterSet>> GrandpaImpl::getVoters() const {
    /*
     * TODO(kamilsa): PRE-356 Check if voters were updated:
     * We should check if voters received from runtime (through
     * grandpa->grandpa_authorities() runtime entry call) differ from the ones
     * that we obtained from the storage. If so, we should return voter set with
     * incremented voter set and consisting of new voters. Also round number
     * should be reset to 0
     */
    OUTCOME_TRY(voters_encoded, storage_->get(storage::kAuthoritySetKey));
    OUTCOME_TRY(voter_set, scale::decode<VoterSet>(voters_encoded));
    return std::make_shared<VoterSet>(voter_set);
  }

  outcome::result<CompletedRound> GrandpaImpl::getLastCompletedRound() const {
    auto last_round_encoded_res = storage_->get(storage::kSetStateKey);

    // Saved data exists
    if (last_round_encoded_res.has_value()) {
      return scale::decode<CompletedRound>(last_round_encoded_res.value());
    }

    // Fail at retrieve data
    if (last_round_encoded_res
        != outcome::failure(storage::DatabaseError::NOT_FOUND)) {
      logger_->critical(
          "Can't retrieve last round data: {}. Stopping grandpa execution",
          last_round_encoded_res.error().message());
      return last_round_encoded_res.as_failure();
    }

    // No saved data - make from genesis
    auto genesis_hash_res = storage_->get(storage::kGenesisBlockHashLookupKey);
    if (not genesis_hash_res.has_value()) {
      logger_->critical(
          "Can't retrieve genesis block hash: {}. Stopping grandpa execution",
          genesis_hash_res.error().message());
      return genesis_hash_res.as_failure();
    }

    primitives::BlockHash genesis_hash;
    std::copy(genesis_hash_res.value().begin(),
              genesis_hash_res.value().end(),
              genesis_hash.begin());

    auto round_state = std::make_shared<const RoundState>(RoundState{
        .last_finalized_block = primitives::BlockInfo(1, genesis_hash),
        .best_prevote_candidate = Prevote(1, genesis_hash),
        .best_final_candidate = primitives::BlockInfo(1, genesis_hash),
        .finalized = primitives::BlockInfo(1, genesis_hash)});

    CompletedRound zero_round{.round_number = 0, .state = round_state};

    return std::move(zero_round);
  }

  void GrandpaImpl::executeNextRound() {
    logger_->debug("Execute next round #{} -> #{}",
                   current_round_->roundNumber(),
                   current_round_->roundNumber() + 1);
    previous_round_.swap(current_round_);
    previous_round_->end();
    current_round_ = makeNextRound(previous_round_);
    current_round_->play();
  }

  void GrandpaImpl::onVoteMessage(const VoteMessage &msg) {
    std::shared_ptr<VotingRound> target_round = selectRound(msg.round_number);
    if (not target_round) return;

    // get block info
    auto blockInfo = visit_in_place(msg.vote.message, [](const auto &vote) {
      return BlockInfo(vote.block_number, vote.block_hash);
    });

    // get authorities
    const auto &weighted_authorities_res =
        grandpa_api_->authorities(primitives::BlockId(blockInfo.block_number));
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
        [&target_round, &msg](const PrimaryPropose &primary_propose) {
          target_round->onPrimaryPropose(msg.vote);
        },
        [&target_round, &msg](const Prevote &prevote) {
          target_round->onPrevote(msg.vote);
        },
        [&target_round, &msg](const Precommit &precommit) {
          target_round->onPrecommit(msg.vote);
        });
  }

  void GrandpaImpl::onFinalize(const Fin &f) {
    logger_->debug("Received fin message for round: {}", f.round_number);

    std::shared_ptr<VotingRound> target_round = selectRound(f.round_number);
    if (not target_round) return;

    target_round->onFinalize(f);
  }

  void GrandpaImpl::onCompletedRound(
      outcome::result<CompletedRound> completed_round_res) {
    round_id++;

    if (not completed_round_res) {
      logger_->debug("Grandpa round was not finalized: {}",
                     completed_round_res.error().message());
    } else {
      const auto &completed_round = completed_round_res.value();

      logger_->debug("Event OnCompleted for round #{}",
                     completed_round.round_number);

      if (auto put_res = storage_->put(
              storage::kSetStateKey,
              common::Buffer(scale::encode(completed_round).value()));
          not put_res) {
        logger_->error("New round state was not added to the storage");
        return;
      }

      BOOST_ASSERT(storage_->get(storage::kSetStateKey));
    }

    boost::asio::post(*io_context_, [wp = weak_from_this()] {
      if (auto grandpa = wp.lock()) {
        grandpa->executeNextRound();
      }
    });
  }
}  // namespace kagome::consensus::grandpa
