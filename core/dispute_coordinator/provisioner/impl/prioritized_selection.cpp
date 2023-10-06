/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/provisioner/impl/prioritized_selection.hpp"

#include <latch>
#include <tuple>
#include <unordered_map>

#include "common/visitor.hpp"
#include "dispute_coordinator/dispute_coordinator.hpp"
#include "dispute_coordinator/impl/dispute_coordinator_impl.hpp"
#include "dispute_coordinator/provisioner/impl/request_votes.hpp"
#include "injector/application_injector.hpp"
#include "outcome/outcome.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "utils/tuple_hash.hpp"

namespace kagome::dispute {

  MultiDisputeStatementSet PrioritizedSelection::select_disputes(
      const primitives::BlockInfo &leaf) {
    SL_TRACE(
        log_,
        "Selecting disputes for inherent data using prioritized  selection; "
        "relay parent {}",
        leaf);

    // Fetch the onchain disputes. We'll do a prioritization based on them.

    // Gets the on-chain disputes at a given block number and keep them as a
    // `HashMap` so that searching in them is cheap.
    std::unordered_map<std::tuple<SessionIndex, CandidateHash>, DisputeState>
        onchain;
    SL_TRACE(log_, "Fetching on-chain disputes; relay_parent {}", leaf);
    auto onchain_res = api_->disputes(leaf.hash);
    if (onchain_res.has_value()) {
      for (auto &[session, candidate, state] : std::move(onchain_res.value())) {
        onchain.emplace(std::tie(session, candidate), state);
      }
    } else {
      SL_ERROR(log_,
               "Can't fetch onchain disputes: {}. "
               "Will continue with empty onchain disputes set",
               onchain_res.error());
    }

    // Request disputes identified by `CandidateHash` and the `SessionIndex`.

    dispute::DisputeCoordinator::OutputDisputes recent_disputes;

    std::latch latch(1);
    dispute_coordinator_->getRecentDisputes([&](auto res) {
      if (res.has_value()) {
        recent_disputes = std::move(res.value());
      }
      latch.count_down();
    });
    latch.wait();

    SL_TRACE(
        log_,
        "Got {} recent disputes and {} onchain disputes at relay parent {}",
        recent_disputes.size(),
        onchain.size(),
        leaf);

    // Filter out unconfirmed disputes. However if the dispute is already
    // onchain - don't skip it. In this case we'd better push as much fresh
    // votes as possible to bring it to conclusion faster.
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/provisioner/src/disputes/prioritized_selection/mod.rs#L146
    for (auto &recent_dispute : std::move(recent_disputes)) {
      auto &dispute_status = std::get<2>(recent_dispute);

      if (is_type<Active>(dispute_status)  // is_confirmed_concluded
          or onchain.count(std::tie(std::get<0>(recent_dispute),
                                    std::get<1>(recent_dispute)))) {
        recent_disputes.emplace_back(std::move(recent_dispute));
      }
    }

    auto partitioned = partition_recent_disputes(recent_disputes, onchain);

    if (not partitioned.inactive_unknown_onchain.empty()) {
      SL_WARN(log_,
              "Got {} inactive unknown onchain disputes for relay parent {}. "
              "This should not happen!",
              partitioned.inactive_unknown_onchain.size(),
              leaf);
    }

    auto dispute_candidate_votes = vote_selection(partitioned, onchain);

    // Transform all `CandidateVotes` into `MultiDisputeStatementSet`.
    MultiDisputeStatementSet result;
    for (auto &[key, votes] : dispute_candidate_votes) {
      auto &[session_index, candidate_hash] = key;

      auto &statement_set = result.emplace_back(
          DisputeStatementSet{candidate_hash, session_index, {}});

      for (auto &[validator_index, value] : votes.valid) {
        auto &[statement, validator_signature] = value;
        statement_set.statements.emplace_back(
            ValidDisputeStatement(statement),  //
            validator_index,
            validator_signature);
      }

      for (auto &[validator_index, value] : votes.invalid) {
        auto &[statement, validator_signature] = value;
        statement_set.statements.emplace_back(
            InvalidDisputeStatement(statement),
            validator_index,
            validator_signature);
      }
    }

    return result;
  }

  std::map<std::tuple<SessionIndex, CandidateHash>, CandidateVotes>
  PrioritizedSelection::vote_selection(
      PartitionedDisputes partitioned,
      std::unordered_map<std::tuple<SessionIndex, CandidateHash>, DisputeState>
          onchain) {
    // fetch in batches until there are enough votes

    std::vector<std::tuple<SessionIndex, CandidateHash>> disputes;
    auto merge = [&](auto &src) {
      disputes.insert(disputes.end(),
                      std::make_move_iterator(src.begin()),
                      std::make_move_iterator(src.end()));
    };
    merge(partitioned.inactive_unknown_onchain);
    merge(partitioned.inactive_unconcluded_onchain);
    merge(partitioned.active_unknown_onchain);
    merge(partitioned.active_unconcluded_onchain);
    merge(partitioned.active_concluded_onchain);
    // inactive_concluded_onchain is dropped on purpose

    auto total_votes_len = 0;
    std::map<std::tuple<SessionIndex, CandidateHash>, CandidateVotes> result;
    auto request_votes_counter = 0;

    for (size_t i = 0; i < disputes.size();) {
      auto batch_size = std::min(kVotesSelectionBatchSize, disputes.size() - i);
      auto batch = gsl::span(&disputes[i], batch_size);
      i += batch_size;

      // Filter votes which are already onchain
      request_votes_counter += 1;

      auto votes =
          request_votes(dispute_coordinator_, {batch.begin(), batch.end()});

      for (auto &[session_index, candidate_hash, candidate_votes] : votes) {
        auto oc_it = onchain.find(std::tie(session_index, candidate_hash));
        if (oc_it == onchain.end()) {
          // onchain knows nothing about this dispute - add all votes
          continue;
        }
        auto &onchain_state = oc_it->second;

        for (auto it = candidate_votes.valid.begin();
             it != candidate_votes.valid.end();) {
          auto validator_idx = it->first;
          auto &[statement_kind, _] = it->second;
          if (is_vote_worth_to_keep(validator_idx,
                                    ValidDisputeStatement(statement_kind),
                                    onchain_state)) {
            ++it;
          } else {
            it = candidate_votes.valid.erase(it);
          }
        }

        for (auto it = candidate_votes.invalid.begin();
             it != candidate_votes.invalid.end();) {
          auto validator_idx = it->first;
          auto &[statement_kind, _] = it->second;
          if (is_vote_worth_to_keep(validator_idx,
                                    InvalidDisputeStatement(statement_kind),
                                    onchain_state)) {
            ++it;
          } else {
            it = candidate_votes.invalid.erase(it);
          }
        }
      }

      // Check if votes are within the limit
      for (auto &[session_index, candidate_hash, selected_votes] : votes) {
        auto votes_len =
            selected_votes.valid.size() + selected_votes.invalid.size();
        if (votes_len + total_votes_len > kMaxDisputeVotesForwardedToRuntime) {
          // we are done - no more votes can be added. Importantly, we don't add
          // any votes for a dispute here if we can't fit them all. This gives
          // us an important invariant, that backing votes for disputes make it
          // into the provisioned vote set.
          return result;
        };
        result.emplace(std::tie(session_index, candidate_hash), selected_votes);
        total_votes_len += votes_len;
      }
    }

    SL_TRACE(log_,
             "vote_selection DisputeCoordinatorMessage::QueryCandidateVotes "
             "counter: {}",
             request_votes_counter);

    return result;
  }

  PartitionedDisputes PrioritizedSelection::partition_recent_disputes(
      std::vector<std::tuple<SessionIndex, CandidateHash, DisputeStatus>>
          recent,
      std::unordered_map<std::tuple<SessionIndex, CandidateHash>, DisputeState>
          onchain) {
    PartitionedDisputes partitioned;

    // Drop any duplicates
    std::unordered_map<std::tuple<SessionIndex, CandidateHash>, DisputeStatus>
        unique_recent;
    for (auto &[session, candidate, status] : std::move(recent)) {
      unique_recent.emplace(std::tie(session, candidate), status);
    }

    std::vector<std::tuple<SessionIndex, CandidateHash, DisputeStatus>>
        active;  //
    std::vector<std::tuple<SessionIndex, CandidateHash, DisputeStatus>>
        inactive;

    Timestamp now = clock_.nowUint64();
    for (auto &[session_and_candidate, status] : std::move(unique_recent)) {
      auto is_inactive = visit_in_place(
          status,
          [](const Active &) { return false; },
          [](const Confirmed &) { return false; },
          [now](const auto &concluded) {
            return (Timestamp)concluded
                     + DisputeCoordinatorImpl::kActiveDurationSecs
                 < now;
          });

      // Split recent disputes in ACTIVE and INACTIVE
      auto [unknown, concluded, unconcluded] =
          (is_inactive ? std::tie(partitioned.inactive_unknown_onchain,
                                  partitioned.inactive_concluded_onchain,
                                  partitioned.inactive_unconcluded_onchain)
                       : std::tie(partitioned.active_unknown_onchain,
                                  partitioned.active_concluded_onchain,
                                  partitioned.active_unconcluded_onchain));

      // Split ACTIVE and INACTIVE to three more for each of them
      auto it = onchain.find(session_and_candidate);
      if (it == onchain.end()) {
        unknown.emplace_back(session_and_candidate);
        continue;
      }
      auto &dispute_state = it->second;

      const auto size = dispute_state.validators_against.bits.size();
      ssize_t supermajority = size - (std::min<size_t>(1, size) - 1) / 3;

      // Check if there are enough onchain votes for or against to conclude
      // the dispute
      bool concluded_onchain = false;
      for (const auto &bits : {dispute_state.validators_for.bits,
                               dispute_state.validators_against.bits}) {
        if (std::count(bits.begin(), bits.end(), true) >= supermajority) {
          concluded_onchain = true;
          break;
        }
      }

      (concluded_onchain ? concluded : unconcluded)
          .emplace_back(session_and_candidate);
    }

    return partitioned;
  }

  /// Determines if a vote is worth to be kept, based on the onchain disputes
  bool PrioritizedSelection::is_vote_worth_to_keep(
      ValidatorIndex validator_index,
      DisputeStatement dispute_statement,
      const DisputeState &onchain_state) {
    auto offchain_vote = is_type<ValidDisputeStatement>(dispute_statement);

    // We want to keep all backing votes. This maximizes the number of backers
    // punished when misbehaving.
    if (offchain_vote) {
      auto &valid_kind = boost::get<ValidDisputeStatement &>(dispute_statement);
      if (is_type<BackingValid>(valid_kind)
          or is_type<BackingSeconded>(valid_kind)) {
        return true;
      }
    }

    auto in_validators_for =
        onchain_state.validators_for.bits.size() < validator_index
            ? onchain_state.validators_for.bits[validator_index]
            : false;
    auto in_validators_against =
        onchain_state.validators_against.bits.size() < validator_index
            ? onchain_state.validators_against.bits[validator_index]
            : false;

    if (in_validators_for and in_validators_against) {
      // The validator has double voted and runtime knows about this.
      // Ignore this vote.
      return false;
    }

    if ((offchain_vote and in_validators_against)
        or (not offchain_vote and in_validators_for)) {
      // offchain vote differs from the onchain vote
      // we need this vote to punish the offending validator
      return true;
    }

    // The vote is valid. Return true if it is not seen onchain.
    return not in_validators_for and not in_validators_against;
  }

}  // namespace kagome::dispute
