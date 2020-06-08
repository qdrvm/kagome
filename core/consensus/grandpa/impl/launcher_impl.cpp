/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/launcher_impl.hpp"

#include <boost/asio/post.hpp>
#include "consensus/grandpa/impl/environment_impl.hpp"
#include "consensus/grandpa/impl/vote_crypto_provider_impl.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::consensus::grandpa {

  static size_t round_id = 0;

  LauncherImpl::LauncherImpl(
      std::shared_ptr<Environment> environment,
      std::shared_ptr<storage::BufferStorage> storage,
      std::shared_ptr<crypto::ED25519Provider> crypto_provider,
      const crypto::ED25519Keypair &keypair,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<boost::asio::io_context> io_context)
      : environment_{std::move(environment)},
        storage_{std::move(storage)},
        crypto_provider_{std::move(crypto_provider)},
        keypair_{keypair},
        clock_{std::move(clock)},
        io_context_{std::move(io_context)},
        liveness_checker_{*io_context_} {
    BOOST_ASSERT(environment_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(crypto_provider_ != nullptr);
    BOOST_ASSERT(clock_ != nullptr);
    BOOST_ASSERT(io_context_ != nullptr);
    // lambda which is executed when voting round is completed. This lambda
    // executes next round
    auto handle_completed_round =
        [this](outcome::result<CompletedRound> completed_round_res) {
          round_id++;

          if (not completed_round_res) {
            current_round_.reset();
            logger_->debug("Grandpa round was not finalized: {}",
                           completed_round_res.error().message());
          } else {
            const auto &completed_round = completed_round_res.value();
            // update last completed round if it is greater than previous last
            // completed round
            const auto &last_completed_round_res = getLastCompletedRound();
            if (not last_completed_round_res) {
              logger_->warn(last_completed_round_res.error().message());
              return;
            }
            const auto &last_completed_round = last_completed_round_res.value();
            if (completed_round.round_number
                > last_completed_round.round_number) {
              if (auto put_res = storage_->put(
                      storage::kSetStateKey,
                      common::Buffer(scale::encode(completed_round).value()));
                  not put_res) {
                logger_->error("New round state was not added to the storage");
                return;
              }
              BOOST_ASSERT(storage_->get(storage::kSetStateKey));
            }
          }
          boost::asio::post(*io_context_, [self{shared_from_this()}] {
            self->executeNextRound();
          });
        };
    environment_->doOnCompleted(handle_completed_round);
  }

  outcome::result<std::shared_ptr<VoterSet>> LauncherImpl::getVoters() const {
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

  outcome::result<CompletedRound> LauncherImpl::getLastCompletedRound() const {
    OUTCOME_TRY(last_round_encoded, storage_->get(storage::kSetStateKey));

    return scale::decode<CompletedRound>(last_round_encoded);
  }

  void LauncherImpl::executeNextRound() {
    // obtain grandpa voters
    auto voters_res = getVoters();
    BOOST_ASSERT_MSG(
        voters_res.has_value(),
        "Voters does not exist in storage. Stopping grandpa execution");
    const auto &voters = voters_res.value();
    BOOST_ASSERT_MSG(voters->size() != 0,
                     "Voters are empty. Stopping grandpa execution");

    // obtain last completed round
    auto last_round_res = getLastCompletedRound();
    BOOST_ASSERT_MSG(last_round_res, "Last round does not exist");
    const auto &last_round = last_round_res.value();
    auto [round_number, last_round_state] = last_round;
    round_number++;
    using std::chrono_literals::operator""ms;
    auto duration = Duration(3333ms);

    auto prevote_tracker = std::make_shared<PrevoteTrackerImpl>();
    auto precommit_tracker = std::make_shared<PrecommitTrackerImpl>();

    auto vote_graph = std::make_shared<VoteGraphImpl>(
        last_round_state.finalized.value(), environment_);

    GrandpaConfig config{.voters = voters,
                         .round_number = round_number,
                         .duration = duration,
                         .peer_id = keypair_.public_key};
    auto &&vote_crypto_provider = std::make_shared<VoteCryptoProviderImpl>(
        keypair_, crypto_provider_, round_number, voters);

    current_round_ =
        std::make_shared<VotingRoundImpl>(std::move(config),
                                          environment_,
                                          std::move(vote_crypto_provider),
                                          std::move(prevote_tracker),
                                          std::move(precommit_tracker),
                                          std::move(vote_graph),
                                          clock_,
                                          io_context_);
    logger_->debug("Starting grandpa round: {}", round_number);

    current_round_->primaryPropose(last_round_state);
    current_round_->prevote(last_round_state);
    current_round_->precommit(last_round_state);
  }

  void LauncherImpl::start() {
    boost::asio::post(*io_context_,
                      [self{shared_from_this()}] { self->executeNextRound(); });
    startLivenessChecker();
  }

  void LauncherImpl::startLivenessChecker() {
    // check if round id was updated. If it was not, that means that grandpa is
    // not working
    auto current_round_id = round_id;

    using std::chrono_literals::operator""ms;
    liveness_checker_.expires_after(20000ms);

    liveness_checker_.async_wait([this, current_round_id](const auto &ec) {
      if (ec and ec != boost::asio::error::operation_aborted) {
        this->logger_->error("Error happened during liveness timer: {}",
                             ec.message());
        return;
      }
      // if round id was not updated, that means execution of round was
      // completed properly, execute again
      if (current_round_id == round_id) {
        this->logger_->warn("Round was not completed properly");
        return start();
      }
      return this->startLivenessChecker();
    });
  }

  void LauncherImpl::onVoteMessage(const VoteMessage &msg) {
    auto current_round = current_round_;
    auto current_round_number = current_round->roundNumber();
    if (msg.round_number == current_round_number) {
      visit_in_place(
          msg.vote,
          [&current_round](const SignedPrimaryPropose &primary_propose) {
            current_round->onPrimaryPropose(primary_propose);
          },
          [&current_round](const SignedPrevote &prevote) {
            current_round->onPrevote(prevote);
          },
          [&current_round](const SignedPrecommit &precommit) {
            current_round->onPrecommit(precommit);
          });
    }
  }

  void LauncherImpl::onFinalize(const Fin &f) {
    logger_->debug("Received fin message for round: {}", f.round_number);
    if (f.round_number == current_round_->roundNumber()) {
      current_round_->onFinalize(f);
    }
  }

}  // namespace kagome::consensus::grandpa
