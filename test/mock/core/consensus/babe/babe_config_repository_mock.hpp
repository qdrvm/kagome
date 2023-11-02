/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/babe/babe_config_repository.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

  class BabeConfigRepositoryMock : public BabeConfigRepository {
   public:
    MOCK_METHOD(Duration, slotDuration, (), (const, override));

    MOCK_METHOD(EpochLength, epochLength, (), (const, override));

    MOCK_METHOD(
        outcome::result<std::shared_ptr<const primitives::BabeConfiguration>>,
        config,
        (const primitives::BlockInfo &, EpochNumber),
        (const, override));

    MOCK_METHOD(void, warp, (const primitives::BlockInfo &), (override));
  };

}  // namespace kagome::consensus::babe
