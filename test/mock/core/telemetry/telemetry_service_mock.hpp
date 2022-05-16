/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TELEMETRY_SERVICE_MOCK_HPP
#define KAGOME_TELEMETRY_SERVICE_MOCK_HPP

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

    MOCK_METHOD(void, notifyWasSynchronized, (), (override));

    MOCK_METHOD(bool, isEnabled, (), (const override));
  };
}  // namespace kagome::telemetry

#endif  // KAGOME_TELEMETRY_SERVICE_MOCK_HPP
