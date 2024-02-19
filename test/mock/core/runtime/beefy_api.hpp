/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/beefy.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {
  struct BeefyApiMock : BeefyApi {
    MOCK_METHOD(outcome::result<std::optional<primitives::BlockNumber>>,
                genesis,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<std::optional<consensus::beefy::ValidatorSet>>,
                validatorSet,
                (const primitives::BlockHash &),
                (override));
  };
}  // namespace kagome::runtime
