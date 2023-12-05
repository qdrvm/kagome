/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/provisioner/impl/request_votes.hpp"

#include <latch>

#include "dispute_coordinator/dispute_coordinator.hpp"

namespace kagome::dispute {

  std::vector<std::tuple<SessionIndex, CandidateHash, CandidateVotes>>
  request_votes(
      const std::shared_ptr<dispute::DisputeCoordinator> &dispute_coordinator,
      const std::vector<std::tuple<SessionIndex, CandidateHash>> &disputes) {
    // Bounded by block production - `ProvisionerMessage::RequestInherentData`.

    DisputeCoordinator::OutputCandidateVotes candidate_votes;

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/provisioner/src/disputes/mod.rs#L37
    std::latch latch(1);
    dispute_coordinator->queryCandidateVotes(disputes, [&](auto res) {
      if (res.has_value()) {
        candidate_votes = std::move(res.value());
      }
      latch.count_down();
    });
    latch.wait();

    return candidate_votes;
  }

}  // namespace kagome::dispute
