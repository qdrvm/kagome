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

  LauncherImpl::LauncherImpl(
      std::shared_ptr<Environment> environment,
      std::shared_ptr<storage::PersistentBufferMap> storage,
      std::shared_ptr<crypto::ED25519Provider> crypto_provider,
      const crypto::ED25519Keypair &keypair,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<boost::asio::io_context> io_context)
      : environment_{std::move(environment)},
        storage_{std::move(storage)},
        crypto_provider_{std::move(crypto_provider)},
        keypair_{keypair},
        clock_{std::move(clock)},
        io_context_{std::move(io_context)} {
    // lambda which is executed when voting round is completed. This lambda
    // executes next round
    auto handle_completed_round =
        [this](const CompletedRound &completed_round) {
          if (auto put_res = storage_->put(
                  storage::kSetStateKey,
                  common::Buffer(scale::encode(completed_round).value()));
              not put_res) {
            logger_->error("New round state was not added to the storage");
            return;
          }
          BOOST_ASSERT(storage_->get(storage::kSetStateKey));
          boost::asio::post(*io_context_,
                            [self{shared_from_this()}, completed_round] {
                              self->executeNextRound(completed_round);
                            });
        };
    environment_->doOnCompleted(handle_completed_round);
  }

  outcome::result<std::shared_ptr<VoterSet>> LauncherImpl::getVoters() const {
    OUTCOME_TRY(voters_encoded, storage_->get(storage::kAuthoritySetKey));
    OUTCOME_TRY(voter_set, scale::decode<VoterSet>(voters_encoded));
    return std::make_shared<VoterSet>(voter_set);
  }

  outcome::result<CompletedRound> LauncherImpl::getLastRoundNumber() const {
    OUTCOME_TRY(last_round_encoded, storage_->get(storage::kSetStateKey));

    return scale::decode<CompletedRound>(last_round_encoded);
  }

  void LauncherImpl::executeNextRound(const CompletedRound &last_round) {
    auto voters_res = getVoters();
    BOOST_ASSERT_MSG(
        voters_res.has_value(),
        "Voters does not exist in storage. Stopping grandpa execution");
    const auto &voters = voters_res.value();
    BOOST_ASSERT_MSG(voters->size() != 0,
                     "Voters are empty. Stopping grandpa execution");
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
        std::make_shared<VotingRoundImpl>(config,
                                          environment_,
                                          std::move(vote_crypto_provider),
                                          prevote_tracker,
                                          precommit_tracker,
                                          vote_graph,
                                          clock_,
                                          io_context_);
    logger_->info("Starting grandpa round: {}", round_number);

    current_round_->primaryPropose(last_round_state);
    current_round_->prevote(last_round_state);
    current_round_->precommit(last_round_state);
  }

  void LauncherImpl::start() {
    auto last_round_res = getLastRoundNumber();
    BOOST_ASSERT_MSG(last_round_res, "Last round does not exist");
    const auto &last_round = last_round_res.value();
    boost::asio::post(*io_context_, [self{shared_from_this()}, last_round] {
      self->executeNextRound(last_round);
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

  void LauncherImpl::onFin(const Fin &f) {
    if (f.round_number == current_round_->roundNumber()) {
      current_round_->onFin(f);
    }
  }

}  // namespace kagome::consensus::grandpa
