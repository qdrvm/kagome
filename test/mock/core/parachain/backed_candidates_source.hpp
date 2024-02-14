/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "parachain/validator/parachain_processor.hpp"

namespace kagome::parachain {

  struct BackedCandidatesSourceMock : public IBackedCandidatesSource {
    MOCK_METHOD(
        std::vector<network::BackedCandidate>,
        getBackedCandidates,
        (const RelayHash &),
        (override));
  };

}  // namespace kagome::parachain
