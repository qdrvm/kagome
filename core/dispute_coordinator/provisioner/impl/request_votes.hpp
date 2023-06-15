/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_REQUESTVOTES
#define KAGOME_DISPUTE_REQUESTVOTES

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

#endif  // KAGOME_DISPUTE_REQUESTVOTES
