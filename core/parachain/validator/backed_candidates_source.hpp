/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/types.hpp"

namespace kagome::network {
  struct BackedCandidate;
}  // namespace kagome::network

namespace kagome::parachain {
  struct BackedCandidatesSource {
    virtual ~BackedCandidatesSource() = default;
    virtual std::vector<network::BackedCandidate> getBackedCandidates(
        const RelayHash &relay_parent) = 0;
  };
}  // namespace kagome::parachain
