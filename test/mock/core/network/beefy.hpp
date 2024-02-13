/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/beefy/beefy.hpp"

#include <gmock/gmock.h>

namespace kagome::network {
  struct BeefyMock : public Beefy {
    MOCK_METHOD(
        outcome::result<std::optional<consensus::beefy::BeefyJustification>>,
        getJustification,
        (primitives::BlockNumber),
        (const, override));

    MOCK_METHOD(void,
                onJustification,
                (const primitives::BlockHash &, primitives::Justification),
                (override));
  };
}  // namespace kagome::network
