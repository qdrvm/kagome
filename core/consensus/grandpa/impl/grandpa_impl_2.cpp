/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/grandpa_impl_2.hpp"

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

  GrandpaImpl2::GrandpaImpl2(
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
        authority_manager_(std::move(authority_manager)),
        liveness_checker_(*io_context_) {
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

  bool GrandpaImpl2::prepare() {
    // Lambda which is executed when voting round is completed.
    environment_->doOnCompleted(
        [wp = weak_from_this()](
            outcome::result<CompletedRound> completed_round_res) {
          if (auto self = wp.lock()) {
            self->onCompletedRound(completed_round_res);
          }
        });
    return true;
  }

  bool GrandpaImpl2::start() {
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

    const auto &[last_round_number, last_round_state] = last_round_res.value();

    logger_->debug("Grandpa is starting with round #{}", last_round_number + 1);

    current_round = makeInitialRound(
        last_round_number,
        std::make_shared<RoundState>(std::move(last_round_state)));

    // Planning play round
    boost::asio::post(*io_context_, [wp = current_round->weak_from_this()] {
      if (auto round = wp.lock()) {
        round->play();
      }
    });

    readinessCheck();
    return true;
  }

  void GrandpaImpl2::stop() {}

  void GrandpaImpl2::readinessCheck() {
    // Check if round id was updated.
    // If it was not, that means that grandpa is not working

    liveness_checker_.expires_after(std::chrono::seconds(20));
    liveness_checker_.async_wait(
        [wp = weak_from_this(), current_round_id = round_id](const auto &ec) {
          auto grandpa = wp.lock();
          if (not grandpa) {
            return;
          }
          if (ec and ec != boost::asio::error::operation_aborted) {
            grandpa->logger_->error("Error happened during readiness timer: {}",
                                    ec.message());
            return;
          }
          // if round id was not updated, that means execution of round was
          // completed properly, execute again
          if (current_round_id == round_id) {
            grandpa->logger_->warn("Round was not completed properly");
            // Planning play round
            boost::asio::post(*grandpa->io_context_,
                              [wp = grandpa->current_round->weak_from_this()] {
                                if (auto round = wp.lock()) {
                                  round->play();
                                }
                              });
          }
          grandpa->readinessCheck();
        });
  }

  std::shared_ptr<VotingRoundImpl2> GrandpaImpl2::makeInitialRound(
      RoundNumber previous_round_number,
      std::shared_ptr<RoundState> previous_round_state) {
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
        previous_round_state->finalized.value(), environment_);

    GrandpaConfig config{
        .voters = voters,
        .round_number = new_round_number,
        .duration = std::chrono::milliseconds(1000),  // Where is it from?
        .peer_id = keypair_.public_key};

    auto vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair_, crypto_provider_, new_round_number, voters);

    auto new_round = std::make_shared<VotingRoundImpl2>(
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

  std::shared_ptr<VotingRoundImpl2> GrandpaImpl2::makeNextRound(
      const std::shared_ptr<VotingRoundImpl2> &round) {
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


	  auto vote_graph = std::make_shared<VoteGraphImpl>(
			  round->getCurrentState()->finalized.value(), environment_);

    GrandpaConfig config{
        .voters = voters,
        .round_number = new_round_number,
        .duration = std::chrono::milliseconds(1000),  // Where is it from?
        .peer_id = keypair_.public_key};

    auto vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair_, crypto_provider_, new_round_number, voters);

    auto new_round = std::make_shared<VotingRoundImpl2>(
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

  outcome::result<std::shared_ptr<VoterSet>> GrandpaImpl2::getVoters() const {
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

  outcome::result<CompletedRound> GrandpaImpl2::getLastCompletedRound() const {
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

    CompletedRound zero_round;
    zero_round.round_number = 0;
    zero_round.state.last_finalized_block =
        primitives::BlockInfo(0, genesis_hash);
    zero_round.state.prevote_ghost = Prevote(0, genesis_hash);
    zero_round.state.estimate = primitives::BlockInfo(0, genesis_hash);
    zero_round.state.finalized = primitives::BlockInfo(0, genesis_hash);
    return std::move(zero_round);
  }

  void GrandpaImpl2::executeNextRound() {
    previous_round.swap(current_round);
    previous_round->end();
    current_round = makeNextRound(previous_round);
    current_round->play();
  }

  void GrandpaImpl2::onVoteMessage(const VoteMessage &msg) {
    std::shared_ptr<VotingRound> target_round;
    if (current_round && current_round->roundNumber() == msg.round_number) {
      target_round = current_round;
    } else if (previous_round
               && previous_round->roundNumber() == msg.round_number) {
      target_round = previous_round;
    } else {
      return;
    }

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

  void GrandpaImpl2::onFinalize(const Fin &f) {
    logger_->debug("Received fin message for round: {}", f.round_number);

    std::shared_ptr<VotingRound> target_round;
    if (current_round && current_round->roundNumber() == f.round_number) {
      target_round = current_round;
    } else if (previous_round
               && previous_round->roundNumber() == f.round_number) {
      target_round = previous_round;
    } else {
      return;
    }

    target_round->onFinalize(f);
  }

  void GrandpaImpl2::onCompletedRound(
      outcome::result<CompletedRound> completed_round_res) {
    round_id++;

    if (not completed_round_res) {
      logger_->debug("Grandpa round was not finalized: {}",
                     completed_round_res.error().message());
    } else {
      const auto &completed_round = completed_round_res.value();

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
