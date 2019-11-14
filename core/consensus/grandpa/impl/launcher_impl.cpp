/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/launcher_impl.hpp"

#include <boost/asio/post.hpp>
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

  std::shared_ptr<VoterSet> LauncherImpl::getVoters() const {
    auto voters_encoded_res = storage_->get(kAuthoritySetKey);
    if (not voters_encoded_res) {
      // handle error
    }
    auto voter_set_res = scale::decode<VoterSet>(voters_encoded_res.value());
    if (not voter_set_res) {
      // handle error
    }
    return std::make_shared<VoterSet>(voter_set_res.value());
  }

  CompletedRound LauncherImpl::getLastRoundNumber() const {
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
    auto voters = getVoters();
    auto [round_number, last_round_state] = getLastRoundNumber();
    round_number++;

    auto duration = Duration(333);

    auto prevote_tracker = std::make_shared<PrevoteTrackerImpl>();
    auto precommit_tracker = std::make_shared<PrecommitTrackerImpl>();

    auto vote_graph = std::make_shared<VoteGraphImpl>(
        last_round_state.finalized.value(), chain_);

    auto handle_completed_round = [this](
                                      const CompletedRound &completed_round) {
      auto &&encoded_round_state = scale::encode(completed_round).value();
      if (auto put_res =
              storage_->put(kSetStateKey, common::Buffer(encoded_round_state));
          not put_res) {
        // handle error
      }

      boost::asio::post(*io_context_,
                        boost::bind(&LauncherImpl::executeNextRound, this));
    };

    std::shared_ptr<VotingRound> round =
        std::make_shared<VotingRoundImpl>(voters,
                                          round_number,
                                          duration,
                                          keypair_,
                                          prevote_tracker,
                                          precommit_tracker,
                                          chain_,
                                          vote_graph,
                                          gossiper_,
                                          ed_provider_,
                                          clock_,
                                          block_tree_,
                                          io_context_,
                                          handle_completed_round);

    current_round_ = round;

    round->primaryPropose(last_round_state);
    round->prevote(last_round_state);
    round->precommit(last_round_state);
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
