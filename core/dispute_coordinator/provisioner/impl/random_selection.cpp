/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/provisioner/impl/random_selection.hpp"

#include <latch>

#include "dispute_coordinator/dispute_coordinator.hpp"
#include "dispute_coordinator/provisioner/impl/request_votes.hpp"
#include "utils/tuple_hash.hpp"

namespace kagome::dispute {

  MultiDisputeStatementSet RandomSelection::select_disputes() {
    SL_TRACE(log_,
             "Selecting disputes for inherent data using random selection");

    // We use `RecentDisputes` instead of `ActiveDisputes` because redundancy is
    // fine.
    // It's heavier than `ActiveDisputes` but ensures that everything from the
    // dispute window gets on-chain, unlike `ActiveDisputes`.
    // In case of an overload condition, we limit ourselves to active disputes,
    // and fill up to the upper bound of disputes to pass to wasm `fn
    // create_inherent_data`.
    // If the active ones are already exceeding the bounds, randomly select a
    // subset.

    auto recent = request_confirmed_disputes(RequestType::Recent);

    std::vector<std::tuple<SessionIndex, CandidateHash>> disputes;

    if (recent.size() > kMaxDisputesForwardedToRuntime) {
      SL_WARN(log_,
              "Recent disputes are excessive ({} > {}), "
              "reduce to active ones, and selected",
              recent.size(),
              kMaxDisputesForwardedToRuntime);
      auto active = request_confirmed_disputes(RequestType::Active);

      disputes.reserve(kMaxDisputesForwardedToRuntime);

      if (active.size() > kMaxDisputesForwardedToRuntime) {
        extend_by_random_subset_without_repetition(
            disputes, active, kMaxDisputesForwardedToRuntime);
      } else {
        extend_by_random_subset_without_repetition(
            active, recent, kMaxDisputesForwardedToRuntime - active.size());
        disputes = std::move(active);
      }

    } else {
      disputes = std::move(recent);
    }

    // Load all votes for all disputes from the coordinator.
    auto dispute_candidate_votes =
        request_votes(dispute_coordinator_, std::move(disputes));

    // Transform all `CandidateVotes` into `MultiDisputeStatementSet`.
    MultiDisputeStatementSet result;
    for (auto &[session, candidate, votes] : dispute_candidate_votes) {
      auto &statement_set =
          result.emplace_back(DisputeStatementSet{candidate, session, {}});

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

  std::vector<std::tuple<SessionIndex, CandidateHash>>
  RandomSelection::request_confirmed_disputes(
      RandomSelection::RequestType active_or_recent) {
    dispute::DisputeCoordinator::OutputDisputes disputes;

    std::latch latch(1);
    auto cb = [&](auto res) {
      if (res.has_value()) {
        disputes = std::move(res.value());
      }
      latch.count_down();
    };
    if (active_or_recent == RequestType::Recent) {
      dispute_coordinator_->getRecentDisputes(cb);
    } else {
      dispute_coordinator_->getActiveDisputes(cb);
    }
    latch.wait();

    std::vector<std::tuple<SessionIndex, CandidateHash>> result;

    for (const auto &[session, candidate, status] : disputes) {
      if (not is_type<Active>(status)) {
        result.emplace_back(session, candidate);
      }
    }

    return result;
  }

  void RandomSelection::extend_by_random_subset_without_repetition(
      std::vector<std::tuple<SessionIndex, CandidateHash>> &acc,
      std::vector<std::tuple<SessionIndex, CandidateHash>> extension,
      size_t n) {
    std::unordered_set<std::tuple<SessionIndex, CandidateHash>> lut;
    for (auto &ext : acc) {
      lut.emplace(ext);
    }

    std::unordered_set<std::tuple<SessionIndex, CandidateHash>> unique_new;
    for (auto &ext : std::move(extension)) {
      if (lut.count(ext) == 0) {
        unique_new.emplace(ext);
      }
    }

    // we can simply add all
    if (unique_new.size() <= n) {
      acc.insert(acc.end(), unique_new.begin(), unique_new.end());
    } else {
      extension.assign(unique_new.begin(), unique_new.end());
      std::shuffle(extension.begin(), extension.end(), random_);
      extension.resize(n);

      acc.reserve(acc.size() + n);
      acc.insert(acc.end(), extension.begin(), extension.end());
    }

    // assure sorting stays candid according to session index
    std::sort(acc.begin(), acc.begin(), [](const auto &lhs, const auto &rhs) {
      return std::get<0>(lhs) < std::get<0>(rhs);
    });
  }

}  // namespace kagome::dispute
