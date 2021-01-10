/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/grandpa_impl.hpp"

#include <boost/asio/post.hpp>
#include <storage/database_error.hpp>

#include "consensus/grandpa/impl/vote_crypto_provider_impl.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/impl/voting_round_error.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::consensus::grandpa {

  GrandpaImpl::GrandpaImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<Environment> environment,
      std::shared_ptr<storage::BufferStorage> storage,
      std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      const crypto::Ed25519Keypair &keypair,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<authority::AuthorityManager> authority_manager,
      std::shared_ptr<consensus::Babe> babe)
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

    logger_->debug("Grandpa will be started with round #{}",
                   round_state.round_number + 1);

    current_round_ = makeInitialRound(round_state);
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
        self->is_ready_ = true;
        self->current_round_->play();
      }
    });

    executeNextRound();

    return true;
  }

  void GrandpaImpl::stop() {}

  std::shared_ptr<VotingRound> GrandpaImpl::makeInitialRound(
      const MovableRoundState &round_state) {
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

    auto vote_graph = std::make_shared<VoteGraphImpl>(
        round_state.last_finalized_block, environment_);

    GrandpaConfig config{.voters = voters,
                         .round_number = round_state.round_number,
                         .duration = round_time_factor_,
                         .peer_id = keypair_.public_key};

    auto vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair_, crypto_provider_, round_state.round_number, voters);

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
        round_state);

    new_round->end();
    return new_round;
  }

  std::shared_ptr<VotingRound> GrandpaImpl::makeNextRound(
      const std::shared_ptr<VotingRound> &round) {
    logger_->debug("Making next round");

    BOOST_ASSERT(round->finalizedBlock().has_value());

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

    BlockInfo best_block = round->finalizedBlock().value();

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
//                             .prevotes = {},
                             .precommits = {},
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

  void GrandpaImpl::onCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                                     const network::CatchUpRequest &msg) {
    if (not is_ready_) {
      return;
    }
    if (previous_round_ == nullptr) {
      logger_->debug(
          "Catch-up request (since round #{}) received from {} was rejected: "
          "previous round is dummy yet",
          msg.round_number,
          peer_id.toHex());
      return;
    }
    if (previous_round_->voterSetId() != msg.voter_set_id) {
      // Catching up of different set
      logger_->debug(
          "Catch-up request (since round #{}) received from {} was rejected: "
          "voter set is different",
          msg.round_number,
          peer_id.toHex());
      return;
    }
    if (previous_round_->roundNumber() < msg.round_number) {
      // Catching up in to the past
      logger_->debug(
          "Catch-up request (since round #{}) received from {} was rejected: "
          "catching up in to the past",
          msg.round_number,
          peer_id.toHex());
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
      logger_->debug(
          "Catch-up request (since round #{}) received from {} was rejected: "
          "round is not completable",
          msg.round_number,
          peer_id.toHex());
      return;
    }
    if (not previous_round_->finalizable()) {
      logger_->debug(
          "Catch-up request (since round #{}) received from {} was rejected: "
          "round is not finalizable",
          msg.round_number,
          peer_id.toHex());
      return;
    }

    logger_->debug("Catch-up request (since round #{}) received from {}",
                   msg.round_number,
                   peer_id.toHex());
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
      logger_->debug(
          "Catch-up response (till round #{}) received from {} was rejected: "
          "catching up in to the past",
          msg.round_number,
          peer_id.toHex());
      return;
    }
    if (current_round_->voterSetId() != msg.voter_set_id) {
      // Catching up of different set
      logger_->debug(
          "Catch-up response (till round #{}) received from {} was rejected: "
          "voter set is different",
          msg.round_number,
          peer_id.toHex());
      return;
    }

    logger_->debug("Catch-up response (till round #{}) received from {}",
                   msg.round_number,
                   peer_id.toHex());

    MovableRoundState round_state{
        .round_number = msg.round_number,
        .last_finalized_block = msg.best_final_candidate,
//        .prevotes =
//            [&items = msg.prevote_justification.items] {
//              std::vector<VoteVariant> v;
//              std::transform(items.begin(),
//                             items.end(),
//                             std::back_inserter(v),
//                             [](auto &item) { return item; });
//              return v;
//            }(),
        .precommits =
            [&items = msg.precommit_justification.items] {
              std::vector<VoteVariant> v;
              std::transform(items.begin(),
                             items.end(),
                             std::back_inserter(v),
                             [](auto &item) { return item; });
              return v;
            }(),
        .finalized = msg.best_final_candidate};

    auto round = makeInitialRound(round_state);
    if (round == nullptr) {
      // Can't make round
      return;
    }

    if (current_round_->bestPrevoteCandidate().block_number
        > round->bestFinalCandidate().block_number) {
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
                               const Fin &fin) {
    if (not is_ready_) {
      return;
    }

    if (fin.justification.round_number != fin.round_number) {
      logger_->warn(
          "Round does not correspond to the fin message it belongs to");
      return;
    }

    if (fin.justification.block_info != fin.vote) {
      logger_->warn(
          "Block does not correspond to the fin message it belongs to");
      return;
    }

    auto res =
        applyJustification(fin.justification.block_info, fin.justification);
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

    if (current_round_->roundNumber() > justification.round_number) {
      return VotingRoundError::JUSTIFICATION_FOR_ROUND_IN_PAST;
    }

    if (current_round_->bestPrevoteCandidate().block_number
        > block_info.block_number) {
      return VotingRoundError::JUSTIFICATION_FOR_BLOCK_IN_PAST;
    }

    MovableRoundState round_state{.round_number = justification.round_number,
                                  .last_finalized_block = block_info,
//                                  .prevotes = {},
                                  .precommits = {},
                                  .finalized = block_info};

    auto round = makeInitialRound(round_state);
    assert(round);

    logger_->debug("Rewind grandpa till round #{} by received justification",
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
      logger_->debug("Grandpa round was not finalized: {}",
                     round_state_res.error().message());
      return;
    }

    const auto &round_state = round_state_res.value();

    logger_->debug("Event OnCompleted for round #{}", round_state.round_number);

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
