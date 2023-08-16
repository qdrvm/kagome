/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/provisioner/impl/request_votes.hpp"

#include <future>

#include "dispute_coordinator/dispute_coordinator.hpp"

namespace kagome::dispute {

  std::vector<std::tuple<SessionIndex, CandidateHash, CandidateVotes>>
  request_votes(
      const std::shared_ptr<dispute::DisputeCoordinator> &dispute_coordinator,
      const std::vector<std::tuple<SessionIndex, CandidateHash>> &disputes) {
    // Bounded by block production - `ProvisionerMessage::RequestInherentData`.

    auto promise_res = std::promise<
        outcome::result<DisputeCoordinator::OutputCandidateVotes>>();
    auto res_future = promise_res.get_future();

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/provisioner/src/disputes/mod.rs#L37
    dispute_coordinator->queryCandidateVotes(
        disputes,
        [promise_res = std::ref(promise_res)](
            outcome::result<DisputeCoordinator::OutputCandidateVotes> res) {
          promise_res.get().set_value(std::move(res));
        });

    DisputeCoordinator::OutputCandidateVotes candidate_votes;
    if (not res_future.valid()) {
      // SL_WARN(log_,
      //         "Fetch for approval votes got cancelled, "
      //         "only expected during shutdown!");
    } else {
      auto res = res_future.get();
      if (res.has_error()) {
        // SL_WARN(log_,
        //         "Unable to query candidate votes: {}",
        //         res.error());
      } else {
        candidate_votes = std::move(res.value());
      }
    }

    return candidate_votes;
  }

}  // namespace kagome::dispute
