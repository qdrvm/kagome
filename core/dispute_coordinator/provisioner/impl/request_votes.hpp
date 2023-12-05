/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "dispute_coordinator/types.hpp"

namespace kagome::dispute {
  class DisputeCoordinator;
}

namespace kagome::dispute {

  /// Request the relevant dispute statements for a set of disputes identified
  /// by `CandidateHash` and the `SessionIndex`.
  std::vector<std::tuple<SessionIndex, CandidateHash, CandidateVotes>>
  request_votes(
      const std::shared_ptr<dispute::DisputeCoordinator> &dispute_coordinator,
      const std::vector<std::tuple<SessionIndex, CandidateHash>> &disputes);

}  // namespace kagome::dispute
