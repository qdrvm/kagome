/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/launcher_impl.hpp"

#include <boost/asio/post.hpp>
#include "consensus/grandpa/impl/environment_impl.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus::grandpa {

  //  const VERSION_KEY: &[u8] = b"grandpa_schema_version";
  //  const SET_STATE_KEY: &[u8] = b"grandpa_completed_round";
  //  const AUTHORITY_SET_KEY: &[u8] = b"grandpa_voters";
  //  const CONSENSUS_CHANGES_KEY: &[u8] = b"grandpa_consensus_changes";

  const static auto kAuthoritySetKey = common::Buffer().put("grandpa_voters");
  const static auto kSetStateKey =
      common::Buffer().put("grandpa_completed_round");

  LauncherImpl::LauncherImpl(
      std::shared_ptr<Environment> environment,
      std::shared_ptr<storage::trie::TrieDb> storage,
      std::shared_ptr<VoteCryptoProvider> vote_crypto_provider,
      Id id,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<boost::asio::io_context> io_context)
      : environment_{std::move(environment)},
        storage_{std::move(storage)},
        vote_crypto_provider_{std::move(vote_crypto_provider)},
        id_{id},
        clock_{std::move(clock)},
        io_context_{std::move(io_context)} {
    // lambda which is executed when voting round is completed. This lambda
    // executes next round
    auto handle_completed_round = [this](
                                      const CompletedRound &completed_round) {
      auto &&encoded_round_state = scale::encode(completed_round).value();
      if (auto put_res =
              storage_->put(kSetStateKey, common::Buffer(encoded_round_state));
          not put_res) {
        logger_->error("New round state was not added to the storage");
        return;
      }

      boost::asio::post(*io_context_,
                        boost::bind(&LauncherImpl::executeNextRound, this));
    };
    environment_->onCompleted(handle_completed_round);
  }

  outcome::result<std::shared_ptr<VoterSet>> LauncherImpl::getVoters() const {
    OUTCOME_TRY(voters_encoded, storage_->get(kAuthoritySetKey));
    OUTCOME_TRY(voter_set, scale::decode<VoterSet>(voters_encoded));
    return std::make_shared<VoterSet>(voter_set);
  }

  outcome::result<CompletedRound> LauncherImpl::getLastRoundNumber() const {
    auto last_round_encoded_res = storage_->get(kSetStateKey);
    if (not last_round_encoded_res) {
      // handle error
    }

    auto last_round_res =
        scale::decode<CompletedRound>(last_round_encoded_res.value());
    if (not last_round_res) {
      // handle
    }
    return last_round_res.value();
  }

  void LauncherImpl::executeNextRound() {
    auto voters_res = getVoters();
    if (not voters_res.has_value()) {
      logger_->error(
          "Voters does not exist in storage. Stopping grandpa execution");
      return;
    }
    const auto &voters = voters_res.value();
    auto last_round_res = getLastRoundNumber();
    if (not last_round_res.has_value()) {
      logger_->error(
          "Last round does not exist in storage. Stopping grandpa execution");
      return;
    }
    auto [round_number, last_round_state] = last_round_res.value();
    round_number++;

    auto duration = Duration(333);

    auto prevote_tracker = std::make_shared<PrevoteTrackerImpl>();
    auto precommit_tracker = std::make_shared<PrecommitTrackerImpl>();

    auto vote_graph = std::make_shared<VoteGraphImpl>(
        last_round_state.finalized.value(), environment_);

    GrandpaConfig config{.voters = voters,
                         .round_number = round_number,
                         .duration = duration,
                         .peer_id = id_};

    current_round_ = std::make_shared<VotingRoundImpl>(config,
                                                       environment_,
                                                       vote_crypto_provider_,
                                                       prevote_tracker,
                                                       precommit_tracker,
                                                       vote_graph,
                                                       clock_,
                                                       io_context_);

    current_round_->primaryPropose(last_round_state);
    current_round_->prevote(last_round_state);
    current_round_->precommit(last_round_state);
  }

  void LauncherImpl::start() {
    boost::asio::post(*io_context_,
                      boost::bind(&LauncherImpl::executeNextRound, this));
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

}  // namespace kagome::consensus::grandpa
