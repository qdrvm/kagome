/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "telemetry/service.hpp"

namespace kagome::telemetry {

  class TelemetryServiceMock : public TelemetryService {
    MOCK_METHOD(void,
                setGenesisBlockHash,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(void,
                notifyBlockImported,
                (const primitives::BlockInfo &, telemetry::BlockOrigin),
                (override));

    MOCK_METHOD(void,
                notifyBlockFinalized,
                (const primitives::BlockInfo &),
                (override));

    MOCK_METHOD(void, pushBlockStats, (), (override));

    MOCK_METHOD(void, notifyWasSynchronized, (), (override));

    MOCK_METHOD(bool, isEnabled, (), (const override));
  };
}  // namespace kagome::telemetry
